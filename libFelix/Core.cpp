#include "pch.hpp"
#include "Core.hpp"
#include "CPU.hpp"
#include "Cartridge.hpp"
#include "ComLynx.hpp"
#include "ComLynxWire.hpp"
#include "Mikey.hpp"
#include "InputFile.hpp"
#include "ImageBS93.hpp"
#include "ImageBIOS.hpp"
#include "ImageCart.hpp"
#include "IEscape.hpp"
#include "Log.hpp"
#include "DefaultROM.hpp"
#include "KernelEscape.hpp"
#include "TraceHelper.hpp"

static constexpr uint32_t BAD_LAST_ACCESS_PAGE = ~0;

Core::Core( std::shared_ptr<ComLynxWire> comLynxWire, std::shared_ptr<IVideoSink> videoSink, std::shared_ptr<IInputSource> inputSource, std::span<InputFile> inputs ) : mRAM{}, mROM{}, mPageTypes{}, mEscapes{}, mCurrentTick{}, mSamplesRemainder{}, mActionQueue{},
  mCpu{ std::make_shared<CPU>() }, mTraceHelper{ std::make_shared<TraceHelper>() }, mCartridge{ std::make_shared<Cartridge>( std::make_shared<ImageCart>(), mTraceHelper ) }, mComLynx{ std::make_shared<ComLynx>( comLynxWire ) }, mComLynxWire{ comLynxWire }, mMikey{ std::make_shared<Mikey>( *this, *mComLynx, videoSink ) }, mSuzy{ std::make_shared<Suzy>( *this, inputSource ) },
  mMapCtl{}, mLastAccessPage{ BAD_LAST_ACCESS_PAGE }, mDMAAddress{}, mFastCycleTick{ 4 }, mPatchMagickCodeAccumulator{}, mResetRequestDuringSpriteRendering{}, mSuzyRunning{}, mGlobalSamplesEmitted{}, mGlobalSamplesEmittedSnapshot{}, mGlobalSamplesEmittedPerFrame{}
{
  std::copy( gDefaultROM.cbegin(), gDefaultROM.cend(), mROM.begin() );

  mEscapes[0xf] = std::make_shared<KernelEscape>( mTraceHelper );

  for ( size_t i = 0; i < mPageTypes.size(); ++i )
  {
    switch ( i )
    {
      case 0xff:
      case 0xfe:
        mPageTypes[i] = PageType::KERNEL;
        break;
      case 0xfd:
        mPageTypes[i] = PageType::MIKEY;
        break;
      case 0xfc:
        mPageTypes[i] = PageType::SUZY;
        break;
      default:
        mPageTypes[i] = PageType::RAM;
        break;
    }
  }

  for ( auto const& input : inputs )
  {
    injectFile( input );
  }
}

void Core::injectFile( InputFile const & file )
{
  switch ( file.getType() )
  {
  case InputFile::FileType::BS93:
    if ( auto runAddress = file.getBS93()->load( { mRAM.data(), mRAM.size() } ) )
    {
      pulseReset( runAddress );
    }
    break;
  case InputFile::FileType::BIOS:
    file.getBIOS()->load( { mROM.data(), mROM.size() } );
    pulseReset();
    break;
  case InputFile::FileType::CART:
    mCartridge = std::make_shared<Cartridge>( file.getCart(), mTraceHelper );
    pulseReset();
    break;
  default:
    break;
  }
}

void Core::setLog( std::filesystem::path const & path, uint64_t startCycle )
{
  mCpu->setLog( path, startCycle, mTraceHelper );
}

void Core::setEscape( size_t idx, std::shared_ptr<IEscape> esc )
{
  if ( idx >= 0x10 )
  {
    L_ERROR << "Bad escape index " << idx;
    return;
  }

  mEscapes[idx] = std::move( esc );
}

void Core::pulseReset( std::optional<uint16_t> resetAddress )
{
  if ( resetAddress )
  {
    writeMAPCTL( 0x08 );  //enable RAM in vector space
    mRAM[0xfffc] = *resetAddress & 0xff;
    mRAM[0xfffd] = *resetAddress >> 8;
  }
  else
  {
    writeMAPCTL( 0x00 );  //disable RAM in vector space
  }

  if ( mSuzyProcess )
  {
    mResetRequestDuringSpriteRendering = true;
  }
  else
  {
    assertInterrupt( CPUState::I_RESET, mCurrentTick );
    desertInterrupt( CPUState::I_RESET, mCurrentTick + 5 * 10 );  //asserting RESET for 10 cycles to make sure none will miss it
  }
}

Core::~Core()
{
}

void Core::requestDisplayDMA( uint64_t tick, uint16_t address )
{
  mDMAAddress = address;
  mActionQueue.push( { Action::DISPLAY_DMA, tick } );
}

void Core::runSuzy()
{
  mSuzyRunning = true;
  if ( !mSuzyProcess )
    mSuzyProcess = mSuzy->suzyProcess();
}

void Core::assertInterrupt( int mask, std::optional<uint64_t> tick )
{
  if ( ( mask & CPUState::I_IRQ ) != 0 )
  {
    if ( ( mCpu->interruptedMask() & CPUState::I_IRQ ) == 0 )
    {
      mActionQueue.push( { Action::ASSERT_IRQ, tick.value_or( mCurrentTick ) } );
    }
  }
  else if ( ( mask & CPUState::I_RESET ) != 0 )
  {
    mActionQueue.push( { Action::ASSERT_RESET, tick.value_or( mCurrentTick ) } );
  }
  else
  {
    assert( !"Unsupported interrupt" );
  }
}

void Core::desertInterrupt( int mask, std::optional<uint64_t> tick )
{
  if ( ( mask & CPUState::I_IRQ ) != 0 )
  {
    mActionQueue.push( { Action::DESERT_IRQ, tick.value_or( mCurrentTick ) } );
    return;
  }
  else if ( ( mask & CPUState::I_RESET ) != 0 )
  {
    mActionQueue.push( { Action::DESERT_RESET, tick.value_or( mCurrentTick ) } );
    return;
  }
  else
  {
    assert( !"Unsupported interrupt" );
  }
}

Core::SequencedActionResult Core::executeSequencedAction( SequencedAction seqAction )
{
  auto action = seqAction.getAction();

  switch ( action )
  {
  case Action::DISPLAY_DMA:
    mMikey->setDMAData( mCurrentTick, *(uint64_t *)( mRAM.data() + mDMAAddress ) );
    mCurrentTick += 6 * mFastCycleTick + 2 * 5;
    break;
  case Action::FIRE_TIMER0:
  case Action::FIRE_TIMER1:
  case Action::FIRE_TIMER2:
  case Action::FIRE_TIMER3:
  case Action::FIRE_TIMER4:
  case Action::FIRE_TIMER5:
  case Action::FIRE_TIMER6:
  case Action::FIRE_TIMER7:
  case Action::FIRE_TIMER8:
  case Action::FIRE_TIMER9:
  case Action::FIRE_TIMERA:
  case Action::FIRE_TIMERB:
  case Action::FIRE_TIMERC:
    if ( auto newAction = mMikey->fireTimer( seqAction.getTick(), (int)action - (int)Action::FIRE_TIMER0 ) )
    {
      mActionQueue.push( newAction );
    }
    break;
  case Action::ASSERT_IRQ:
    mCpu->assertInterrupt( CPUState::I_IRQ );
    break;
  case Action::ASSERT_RESET:
    mCpu->assertInterrupt( CPUState::I_RESET );
    break;
  case Action::DESERT_IRQ:
    mCpu->desertInterrupt( CPUState::I_IRQ );
    break;
  case Action::DESERT_RESET:
    mCpu->desertInterrupt( CPUState::I_RESET );
    break;
  case Action::SAMPLE_AUDIO:
    mOutputSamples[mSamplesEmitted++] = mMikey->sampleAudio();
    mGlobalSamplesEmitted += 1;
    if ( mSamplesEmitted >= mOutputSamples.size() )
      return SequencedActionResult::BAIL_OUT;
    enqueueSampling();
    break;
  case Action::BATCH_END:
    return SequencedActionResult::BAIL_OUT;
  default:
    assert( false );
    break;
  }
  return SequencedActionResult::CARRY_ON;
}

bool Core::executeSuzyAction()
{
  if ( !mSuzyProcess || !mSuzyRunning )
    return true;

  if ( mCpu->interruptedMask() != 0 )
  {
    mSuzyRunning = false;
    return true;
  }

  mSuzyProcessRequest = mSuzyProcess->advance();

  switch ( mSuzyProcessRequest->type )
  {
  case ISuzyProcess::Request::FINISH:
    mSuzyRunning = false;
    mMikey->suzyDone();
    mSuzyProcess.reset();
    //workaround to problem with resetting during Suzy activity
    if ( mResetRequestDuringSpriteRendering )
    {
      mResetRequestDuringSpriteRendering = false;
      pulseReset();
    }
    break;
  case ISuzyProcess::Request::READ:
  case ISuzyProcess::Request::FETCHSCB:
    mSuzyProcess->respond( mRAM[mSuzyProcessRequest->addr] );
    mCurrentTick += 5ull; //read byte
    break;
  case ISuzyProcess::Request::READ4:
  case ISuzyProcess::Request::READPAL:
    if ( mSuzyProcessRequest->addr <= 0xfffc )
      mSuzyProcess->respond( *( (uint32_t const *)( mRAM.data() + mSuzyProcessRequest->addr ) ) );
    mCurrentTick += 5ull + 3 * mFastCycleTick;  //read 4 bytes
    break;
  case ISuzyProcess::Request::WRITE:
  case ISuzyProcess::Request::WRITEFRED:
    mRAM[mSuzyProcessRequest->addr] = (uint8_t)mSuzyProcessRequest->value;
    mCurrentTick += 5ull; //write byte
    break;
  case ISuzyProcess::Request::COLRMW:
    {
      if ( mSuzyProcessRequest->addr > 0xfffc )
        break;

      const uint32_t u16 = mSuzyProcessRequest->value;
      const uint32_t u32 = u16 | ( u16 << 16 );
      const uint32_t maskedU32 = u32 & mSuzyProcessRequest->mask;

      const uint32_t value = *( (uint32_t const*)( mRAM.data() + mSuzyProcessRequest->addr ) );
      const uint32_t maskedValue = value & ~mSuzyProcessRequest->mask;
      const uint32_t outValue = value & mSuzyProcessRequest->mask;

      *( (uint32_t *)( mRAM.data() + mSuzyProcessRequest->addr ) ) = maskedValue | maskedU32;

      mSuzyProcess->respond( outValue );
    }
    mCurrentTick += 5ull + 7 * mFastCycleTick;  //read 4 bytes & write 4 bytes
    break;
  case ISuzyProcess::Request::VIDRMW:
    {
      auto value = mRAM[mSuzyProcessRequest->addr] & mSuzyProcessRequest->mask | mSuzyProcessRequest->value;
      mRAM[mSuzyProcessRequest->addr] = (uint8_t)value;
    }
    mCurrentTick += 5ull + mFastCycleTick;  //read & write byte
    break;
  case ISuzyProcess::Request::XOR:
    {
      auto value = mRAM[mSuzyProcessRequest->addr] ^ mSuzyProcessRequest->value;
      mRAM[mSuzyProcessRequest->addr] = (uint8_t)value;
    }
    mCurrentTick += 5ull + mFastCycleTick; //read & write byte
    break;
  }

  return false;
}

void Core::executeCPUAction()
{
  auto const& req = mCpu->advance();

  auto pageType = mPageTypes[req.address >> 8];

  enum class CPUAction
  {
    FETCH_OPCODE_RAM = 0,
    FETCH_OPERAND_RAM,
    READ_RAM,
    WRITE_RAM,
    DISCARD_READ_RAM,
    FETCH_OPCODE_SUZY,
    FETCH_OPERAND_SUZY,
    READ_SUZY,
    WRITE_SUZY,
    DISCARD_READ_SUZY,
    FETCH_OPCODE_MIKEY,
    FETCH_OPERAND_MIKEY,
    READ_MIKEY,
    WRITE_MIKEY,
    DISCARD_READ_MIKEY,
    FETCH_OPCODE_KENREL,
    FETCH_OPERAND_KENREL,
    READ_KENREL,
    WRITE_KENREL,
    DISCARD_READ_KERNEL
  };

  CPUAction action = (CPUAction)( (int)req.type + (int)pageType );

  switch ( action )
  {
  case CPUAction::FETCH_OPCODE_RAM:
    mCpu->respond( mCurrentTick, mRAM[req.address] );
    mCurrentTick += fetchTiming( req.address );
    break;
  case CPUAction::FETCH_OPERAND_RAM:
    mCpu->respond( mRAM[req.address] );
    mCurrentTick += fetchTiming( req.address );
    break;
  case CPUAction::READ_RAM:
    mCpu->respond( mRAM[req.address] );
    mCurrentTick += readTiming( req.address );
    break;
  case CPUAction::WRITE_RAM:
    mRAM[req.address] = req.value;
    mCurrentTick += writeTiming( req.address );
    break;
  case CPUAction::FETCH_OPCODE_KENREL:
    mCpu->respond( mCurrentTick, readKernel( req.address & 0x1ff ) );
    mCurrentTick += fetchTiming( req.address );
    break;
  case CPUAction::FETCH_OPERAND_KENREL:
    mCpu->respond( readKernel( req.address & 0x1ff ) );
    mCurrentTick += fetchTiming( req.address );
    break;
  case CPUAction::READ_KENREL:
    mCpu->respond( readKernel( req.address & 0x1ff ) );
    mCurrentTick += readTiming( req.address );
    break;
  case CPUAction::WRITE_KENREL:
    writeKernel( req.address & 0x1ff, req.value );
    mCurrentTick += writeTiming( req.address );
    break;
  case CPUAction::FETCH_OPCODE_SUZY:
    //no code in Suzy napespace. Should trigger emulation break
    mCurrentTick = mSuzy->requestRead( mCurrentTick, req.address );
    mCpu->respond( mCurrentTick, mSuzy->read( req.address ) );
    break;
  case CPUAction::FETCH_OPERAND_SUZY:
    [[fallthrough]];
  case CPUAction::READ_SUZY:
    mCurrentTick = mSuzy->requestRead( mCurrentTick, req.address );
    mCpu->respond( mSuzy->read( req.address ) );
    mLastAccessPage = BAD_LAST_ACCESS_PAGE;
    break;
  case CPUAction::WRITE_SUZY:
    mCurrentTick = mSuzy->requestWrite( mCurrentTick, req.address );
    mSuzy->write( req.address, req.value );
    mLastAccessPage = BAD_LAST_ACCESS_PAGE;
    break;
  case CPUAction::FETCH_OPCODE_MIKEY:
    //no code in Suzy napespace. Should trigger emulation break
    mCurrentTick = mMikey->requestAccess( mCurrentTick, req.address );
    mCpu->respond( mCurrentTick, mMikey->read( req.address ) );
    break;
  case CPUAction::FETCH_OPERAND_MIKEY:
    [[fallthrough]];
  case CPUAction::READ_MIKEY:
    mCurrentTick = mMikey->requestAccess( mCurrentTick, req.address );
    mCpu->respond( mMikey->read( req.address ) );
    mLastAccessPage = BAD_LAST_ACCESS_PAGE;
    break;
  case CPUAction::WRITE_MIKEY:
    mCurrentTick = mMikey->requestAccess( mCurrentTick, req.address );
    if ( auto mikeyAction = mMikey->write( req.address, req.value ) )
    {
      mActionQueue.push( mikeyAction );
    }
    mLastAccessPage = BAD_LAST_ACCESS_PAGE;
    break;
  case CPUAction::DISCARD_READ_RAM:
    [[fallthrough]];
  case CPUAction::DISCARD_READ_KERNEL:
    mCurrentTick += readTiming( req.address );
    handlePatch( req.address );
    break;
  case CPUAction::DISCARD_READ_SUZY:
    mCurrentTick = mSuzy->requestRead( mCurrentTick, req.address );
    handlePatch( req.address );
    mLastAccessPage = BAD_LAST_ACCESS_PAGE;
    break;
  case CPUAction::DISCARD_READ_MIKEY:
    mCurrentTick = mMikey->requestAccess( mCurrentTick, req.address );
    handlePatch( req.address );
    mLastAccessPage = BAD_LAST_ACCESS_PAGE;
    break;
  }
}

void Core::handlePatch( uint16_t address )
{
  mPatchMagickCodeAccumulator <<= 16;
  mPatchMagickCodeAccumulator |= address;

  if ( mPatchMagickCodeAccumulator >> 12 == 0xc0ffeaa2ab1ca )
  {
    int selector = ( address >> 8 ) & 0xf;
    if ( mEscapes[selector] )
    {
      struct Accessor : public IEscape::IAccessor
      {
        Accessor( std::array<MemU, 65536> & ram, Mikey & mikey, Suzy & suzy, CPUState & state, ActionQueue & actionQueue, uint64_t currentTick ) : currentTick{ currentTick }, ram{ ram }, mikey{ mikey }, suzy{ suzy }, s{ state }, actionQueue{ actionQueue }{}

        uint64_t currentTick;
        std::array<MemU, 65536> & ram;
        Mikey & mikey;
        Suzy & suzy;
        CPUState & s;
        ActionQueue & actionQueue;
        uint8_t readRAM( uint16_t address ) const override
        {
          return ram[address];
        }
        void writeRAM( uint16_t address, uint8_t value ) override
        {
          ram[address] = value;
        }

        uint8_t readMikey( uint16_t address ) const override
        {
          mikey.requestAccess( currentTick, address );
          return mikey.read( address );
        }
        void writeMikey( uint16_t address, uint8_t value ) override
        {
          mikey.requestAccess( currentTick, address );
          if ( auto mikeyAction = mikey.write( address, value ) )
          {
            actionQueue.push( mikeyAction );
          }
        }
        uint8_t readSuzy( uint16_t address ) const override
        {
          suzy.requestRead( currentTick, address );
          return suzy.read( address );
        }
        void writeSuzy( uint16_t address, uint8_t value ) override
        {
          suzy.requestWrite( currentTick, address );
          suzy.write( address, value );
        }

        CPUState & state() override
        {
          return s;
        }
      } acc{ mRAM, *mMikey, *mSuzy, mCpu->state(), mActionQueue, mCurrentTick };

      mEscapes[selector]->call( address & 0xff, acc );
    }
    else
    {
      L_WARNING << "Called empty patch " << selector << " with argument " << ( address & 0xff );
    }
  }
}


void Core::setAudioOut( int sps, std::span<AudioSample> outputBuffer )
{
  mSPS = sps;
  mOutputSamples = outputBuffer;
  mSamplesEmitted = 0;
  enqueueSampling();
}

void Core::enqueueSampling()
{
  int ticks = 16000000 / mSPS;
  mSamplesRemainder += 16000000 % mSPS;
  if ( mSamplesRemainder > mSPS )
  {
    mSamplesRemainder %= mSPS;
    ticks += 1;
  }

  mActionQueue.push( { Action::SAMPLE_AUDIO, mCurrentTick + ticks } );
}

int Core::advanceAudio()
{
  for ( ;; )
  {
    if ( mActionQueue.head().getTick() <= mCurrentTick )
    {
      switch ( executeSequencedAction( mActionQueue.pop() ) )
      {
      case SequencedActionResult::CARRY_ON:
        if ( mSamplesEmitted >= mOutputSamples.size() )
          return 0;
        else
          break;
      case SequencedActionResult::BAIL_OUT:
        return 1;
      }
    }
    else
    {
      if ( executeSuzyAction() )
      {
        executeCPUAction();
      }
    }
  }
}

void Core::enterMonitor()
{
}

int64_t Core::globalSamplesEmittedPerFrame() const
{
  return mGlobalSamplesEmittedPerFrame;
}

ImageCart::Rotation Core::rotation() const
{
  return mCartridge ? mCartridge->rotation() : ImageCart::Rotation{};
}

Cartridge & Core::getCartridge()
{
  assert( mCartridge );
  return *mCartridge;
}

void Core::newLine( int rowNr )
{

  if ( rowNr == 0 )
  {
    mGlobalSamplesEmittedPerFrame = mGlobalSamplesEmitted - mGlobalSamplesEmittedSnapshot;
    mGlobalSamplesEmittedSnapshot = mGlobalSamplesEmitted;
  }
}

std::shared_ptr<TraceHelper> Core::getTraceHelper() const
{
  return mTraceHelper;
}

uint64_t Core::fetchTiming( uint16_t address )
{
  uint32_t page = ( address >> 8 );
  if ( page == mLastAccessPage )
  {
    return mFastCycleTick;
  }
  else
  {
    mLastAccessPage = page;
    return 5;
  }
}

uint64_t Core::readTiming( uint16_t address )
{
  mLastAccessPage = BAD_LAST_ACCESS_PAGE;
  return 5;
}

uint64_t Core::writeTiming( uint16_t address )
{
  mLastAccessPage = BAD_LAST_ACCESS_PAGE;
  return 5;
}

uint8_t Core::readKernel( uint16_t address )
{
  if ( address >= 0x1fa )
  {
    MemU * ptr = mMapCtl.vectorSpaceDisable ? ( mRAM.data() + 0xfe00 ) : ( mROM.data() );
    return ptr[address];
  }
  else if ( address < 0x1f8 )
  {
    MemU * ptr = mMapCtl.kernelDisable ? ( mRAM.data() + 0xfe00 ) : ( mROM.data() );
    return ptr[address];
  }
  else if ( address == 0x1f9 )
  {
    return 0xf0 | //high nibble of MAPCTL is set
      ( mMapCtl.vectorSpaceDisable ? 0x08 : 0x00 ) |
      ( mMapCtl.kernelDisable ? 0x04 : 0x00 ) |
      ( mMapCtl.mikeyDisable ? 0x02 : 0x00 ) |
      ( mMapCtl.suzyDisable ? 0x01 : 0x00 );
  }
  else
  {
    //there is always RAM at 0xfff8
    return mRAM[0xfe00 + address];
  }
}

void Core::writeKernel( uint16_t address, uint8_t value )
{
  if ( address >= 0x1fa && mMapCtl.vectorSpaceDisable || address < 0x1f8 && mMapCtl.kernelDisable || address == 0x1f8 )
  {
    mRAM[0xfe00 + address] = value;
  }
  else if ( address == 0x1f9 )
  {
    writeMAPCTL( value );
  }
  else
  {
    return; //ignore write to ROM
  }
}

void Core::writeMAPCTL( uint8_t value )
{
  mMapCtl.sequentialDisable = ( value & 0x80 ) != 0;
  mMapCtl.vectorSpaceDisable = ( value & 0x08 ) != 0;
  mMapCtl.kernelDisable = ( value & 0x04 ) != 0;
  mMapCtl.mikeyDisable = ( value & 0x02 ) != 0;
  mMapCtl.suzyDisable = ( value & 0x01 ) != 0;

  mFastCycleTick = mMapCtl.sequentialDisable ? 5 : 4;
  mPageTypes[0xfe] = mMapCtl.kernelDisable ? PageType::RAM : PageType::KERNEL;
  mPageTypes[0xfd] = mMapCtl.mikeyDisable ? PageType::RAM : PageType::MIKEY;
  mPageTypes[0xfc] = mMapCtl.suzyDisable ? PageType::RAM : PageType::SUZY;
}

uint8_t Core::sampleRam( uint16_t addr ) const
{
  return mRAM[addr];
}

uint64_t Core::tick() const
{
  return mCurrentTick;
}

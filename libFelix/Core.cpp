#include "pch.hpp"
#include "Core.hpp"
#include "CPU.hpp"
#include "Cartridge.hpp"
#include "ComLynx.hpp"
#include "ComLynxWire.hpp"
#include "Mikey.hpp"
#include "InputFile.hpp"
#include "ImageBS93.hpp"
#include "ImageROM.hpp"
#include "ImageCart.hpp"
#include "Log.hpp"
#include "BootROMTraps.hpp"
#include "TraceHelper.hpp"
#include "DebugRAM.hpp"
#include "ScriptDebuggerEscapes.hpp"
#include "VGMWriter.hpp"

uint8_t* gDebugRAM;

static constexpr uint64_t RESET_DURATION = 5 * 10;  //asserting RESET for 10 cycles to make sure none will miss it
static constexpr uint32_t BAD_LAST_ACCESS_PAGE = ~0;

static constexpr bool ENABLE_TRAPS = true;

Core::Core( ImageProperties const& imageProperties, std::shared_ptr<ComLynxWire> comLynxWire, std::shared_ptr<IVideoSink> videoSink,
  std::shared_ptr<IInputSource> inputSource, InputFile inputFile, std::shared_ptr<ImageROM const> bootROM,
  std::shared_ptr<ScriptDebuggerEscapes> scriptDebuggerEscapes ) :
  mRAM{}, mROM{}, mPageTypes{}, mScriptDebugger{ std::make_shared<ScriptDebugger>() }, mCurrentTick{}, mSamplesRemainder{}, mActionQueue{}, mTraceHelper{ std::make_shared<TraceHelper>() }, mCpu{ std::make_shared<CPU>( mTraceHelper ) },
  mCartridge{ std::make_shared<Cartridge>( imageProperties, std::shared_ptr<ImageCart>{}, mTraceHelper ) }, mComLynx{ std::make_shared<ComLynx>( comLynxWire ) }, mComLynxWire{ comLynxWire },
  mMikey{ std::make_shared<Mikey>( *this, *mComLynx, videoSink ) }, mSuzy{ std::make_shared<Suzy>( *this, inputSource ) }, mMapCtl{}, mLastAccessPage{ BAD_LAST_ACCESS_PAGE },
  mDMAAddress{}, mFastCycleTick{ 4 }, mPatchMagickCodeAccumulator{}, mResetRequestDuringSpriteRendering{}, mSuzyRunning{}, mGlobalSamplesEmitted{}, mGlobalSamplesEmittedSnapshot{}, mGlobalSamplesEmittedPerFrame{}, mHaltSuzy{}
{
  gDebugRAM = &mRAM[0];

  for ( size_t i = 0; i < mPageTypes.size(); ++i )
  {
    switch ( i )
    {
      case 0xff:
      case 0xfe:
        mPageTypes[i] = PageType::ROM;
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

  switch ( inputFile.getType() )
  {
  case InputFile::FileType::BS93:
    if ( auto runAddress = inputFile.getBS93()->load( { mRAM.data(), mRAM.size() } ) )
    {
      pulseReset( runAddress );
      initMikeyRegisters( *this );
    }
    setROM( bootROM );
    break;
  case InputFile::FileType::CART:
    mCartridge = std::make_shared<Cartridge>( imageProperties, inputFile.getCart(), mTraceHelper );
    setROM( bootROM );
    pulseReset();
    break;
  default:
    break;
  }

  scriptDebuggerEscapes->populateScriptDebugger( *mScriptDebugger );
}

void Core::setROM( std::shared_ptr<ImageROM const> bootROM )
{
  if ( bootROM )
  {
    bootROM->load( { mROM.data(), mROM.size() } );
    return;
  }

  std::ranges::fill( mROM, 0xff );

  auto setVec = [mem = mROM.data() - 0xfe00]( uint16_t src, uint16_t dst )
  {
    mem[src + 0] = dst & 0xff;
    mem[src + 1] = dst >> 8;
  };

  //reset vector points to reset handler
  setVec( CPU::RESET_VECTOR, BOOTROM_RESET_HANDLER_ADDRESS );
  //reset handler jumps to clear handler
  setVec( BOOTROM_RESET_HANDLER_ADDRESS + 1, BOOTROM_CLEAR_HANDLER_ADDRESS );
  //clear handler jumps to shift handler
  setVec( BOOTROM_CLEAR_HANDLER_ADDRESS + 1, BOOTROM_SHIFT_HANDLER_ADDRESS );
  //decrypt handler jumps to beginning of decrypted rom
  setVec( BOOTROM_DECRYPT_HANDLER_ADDRESS + 1, DECRYPTED_ROM_START_ADDRESS );

  setBootROMTraps( mTraceHelper, *mScriptDebugger );
}

void Core::setLog( std::filesystem::path const & path )
{
  mCpu->setLog( path );
}

void Core::setVGMWriter( std::filesystem::path const& path )
{
  auto writer = std::unique_ptr<VGMWriter>( path.empty() ? nullptr : new VGMWriter{ path } );
  if ( writer )
    writer->init( mCurrentTick );
  mMikey->setVGMWriter( std::move( writer ) );
}

bool Core::isVGMWriter() const
{
  return mMikey->isVGMWriter();
}

void Core::pulseReset( std::optional<uint16_t> resetAddress )
{
  if ( resetAddress )
  {
    writeMAPCTL( 0x08 );  //enable RAM in vector space
    mRAM[CPU::RESET_VECTOR + 0] = *resetAddress & 0xff;
    mRAM[CPU::RESET_VECTOR + 1] = *resetAddress >> 8;
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
    desertInterrupt( CPUState::I_RESET, mCurrentTick + RESET_DURATION );
  }
}

Core::~Core()
{
  gDebugRAM = nullptr;
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

void Core::executeSequencedAction( SequencedAction seqAction )
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
    if ( mSamplesEmitted >= mOutputSamples.size() )
    {
      mCpu->breakNext();
      mHaltSuzy = true;
    }
    else
    {
      mOutputSamples[mSamplesEmitted++] = mMikey->sampleAudio( mCurrentTick );
      mGlobalSamplesEmitted += 1;
      enqueueSampling();
    }
    break;
  case Action::BATCH_END:
    mCpu->breakNext();
    mHaltSuzy = true;
    break;
  case Action::NONE:
    //removed element
    break;
  default:
    assert( false );
    break;
  }
}

bool Core::executeSuzyAction()
{
  if ( !mSuzyRunning || mHaltSuzy )
    return false;

  assert( mSuzyProcess );

  if ( mCpu->interruptedMask() != 0 )
  {
    mSuzyRunning = false;
    return false;
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
      auto ramValue = mRAM[mSuzyProcessRequest->addr];
      auto xorValue = ramValue ^ mSuzyProcessRequest->value;
      mRAM[mSuzyProcessRequest->addr] = (uint8_t)xorValue;
    }
    mCurrentTick += 5ull + mFastCycleTick; //read & write byte
    break;
  }

  return true;
}

CpuBreakType Core::executeCPUAction()
{
  auto const& req = mCpu->advance();

  auto pageType = mPageTypes[req.address >> 8];

  enum class CPUAction
  {
    FETCH_OPCODE_RAM = 0,
    FETCH_OPERAND_RAM,
    READ_RAM,
    WRITE_RAM,
    FETCH_OPCODE_SUZY,
    FETCH_OPERAND_SUZY,
    READ_SUZY,
    WRITE_SUZY,
    FETCH_OPCODE_MIKEY,
    FETCH_OPERAND_MIKEY,
    READ_MIKEY,
    WRITE_MIKEY,
    FETCH_OPCODE_KENREL,
    FETCH_OPERAND_KENREL,
    READ_KENREL,
    WRITE_KENREL
  };

  CPUAction action = (CPUAction)( (int)req.type + (int)pageType );

  switch ( action )
  {
  case CPUAction::FETCH_OPCODE_RAM:
    mCurrentTick += fetchRAMTiming( req.address );
    return mCpu->respondFetchOpcode( fetchRAM( req.address ) );
  case CPUAction::FETCH_OPERAND_RAM:
    mCpu->respond( readRAM( req.address ) );
    mCurrentTick += fetchRAMTiming( req.address );
    break;
  case CPUAction::READ_RAM:
    mCpu->respond( readRAM( req.address ) );
    mCurrentTick += readTiming( req.address );
    break;
  case CPUAction::WRITE_RAM:
    writeRAM( req.address, req.value );
    mCurrentTick += writeTiming( req.address );
    break;
  case CPUAction::FETCH_OPCODE_KENREL:
    mCurrentTick += fetchROMTiming( req.address );
    return mCpu->respondFetchOpcode( readROM( req.address & 0x1ff, true ) );
  case CPUAction::FETCH_OPERAND_KENREL:
    mCpu->respond( readROM( req.address & 0x1ff, false ) );
    mCurrentTick += fetchROMTiming( req.address );
    break;
  case CPUAction::READ_KENREL:
    mCpu->respond( readROM( req.address & 0x1ff, false ) );
    mCurrentTick += readTiming( req.address );
    break;
  case CPUAction::WRITE_KENREL:
    writeROM( req.address & 0x1ff, req.value );
    mCurrentTick += writeTiming( req.address );
    break;
  case CPUAction::FETCH_OPCODE_SUZY:
    //no code in Suzy napespace. Should trigger emulation break
    mCurrentTick = mSuzy->requestRead( mCurrentTick, req.address );
    return mCpu->respondFetchOpcode( readSuzy( req.address ) );
  case CPUAction::FETCH_OPERAND_SUZY:
    [[fallthrough]];
  case CPUAction::READ_SUZY:
    mCurrentTick = mSuzy->requestRead( mCurrentTick, req.address );
    mCpu->respond( readSuzy( req.address ) );
    mLastAccessPage = BAD_LAST_ACCESS_PAGE;
    break;
  case CPUAction::WRITE_SUZY:
    mCurrentTick = mSuzy->requestWrite( mCurrentTick, req.address );
    writeSuzy( req.address, req.value );
    mLastAccessPage = BAD_LAST_ACCESS_PAGE;
    break;
  case CPUAction::FETCH_OPCODE_MIKEY:
    //no code in Suzy napespace. Should trigger emulation break
    mCurrentTick = mMikey->requestAccess( mCurrentTick, req.address );
    return mCpu->respondFetchOpcode( readMikey( req.address ) );
  case CPUAction::FETCH_OPERAND_MIKEY:
    [[fallthrough]];
  case CPUAction::READ_MIKEY:
    mCurrentTick = mMikey->requestAccess( mCurrentTick, req.address );
    mCpu->respond( readMikey( req.address ) );
    mLastAccessPage = BAD_LAST_ACCESS_PAGE;
    break;
  case CPUAction::WRITE_MIKEY:
    mCurrentTick = mMikey->requestAccess( mCurrentTick, req.address );
    writeMikey( req.address, req.value );
    mLastAccessPage = BAD_LAST_ACCESS_PAGE;
    break;
  }

  return CpuBreakType::NONE;
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

CpuBreakType Core::run( RunMode runMode )
{
  mHaltSuzy = false;

  switch ( runMode )
  {
  case RunMode::STEP_IN:
    mCpu->breakOnStepIn();
    break;
  case RunMode::STEP_OVER:
    mCpu->breakOnStepOver();
    break;
  case RunMode::STEP_OUT:
    mCpu->breakOnStepOut();
    break;
  default:
    mCpu->clearBreak();
    break;
  }

  for ( ;; )
  {
    if ( !mActionQueue.empty() && mActionQueue.headTick() <= mCurrentTick )
    {
      executeSequencedAction( mActionQueue.pop() );
    }
    else if ( !executeSuzyAction() )
    {
      auto cpuBreakType = executeCPUAction();
      if ( cpuBreakType != CpuBreakType::NONE )
        return cpuBreakType;
    }
  }
}

CpuBreakType Core::advanceAudio( int sps, std::span<AudioSample> outputBuffer, RunMode runMode )
{
  mSPS = sps;
  mOutputSamples = outputBuffer;
  mSamplesEmitted = 0;
  CpuBreakType cpuBreakType = CpuBreakType::NONE;

  if ( runMode != RunMode::PAUSE )
  {
    enqueueSampling();
    cpuBreakType = run( runMode );
  }
  else
  {
    mCpu->clearBreak();
  }

  if ( mSamplesEmitted < mOutputSamples.size() )
  {
    mActionQueue.erase( Action::SAMPLE_AUDIO );
    for ( size_t i = mSamplesEmitted; i < mOutputSamples.size(); ++i )
    {
      mOutputSamples[mSamplesEmitted++] = {};
    }
  }

  return cpuBreakType;
}

void Core::enterMonitor()
{
}

int64_t Core::globalSamplesEmittedPerFrame() const
{
  return mGlobalSamplesEmittedPerFrame;
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

std::shared_ptr<ScriptDebugger> Core::getScriptDebugger() const
{
  return mScriptDebugger;
}

uint64_t Core::fetchRAMTiming( uint16_t address )
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

uint64_t Core::fetchROMTiming( uint16_t address )
{
  //Reset handler must burd at least as many cycles as needed to dessert RESET
  return address == 0xff80 ? RESET_DURATION : 5;
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

uint8_t Core::fetchRAM( uint16_t address )
{
  uint8_t sourceByte = mRAM[address];
  if constexpr ( ENABLE_TRAPS )
  {
    uint8_t filteredByte = mScriptDebugger->executeRAM( *this, address, sourceByte );
    return filteredByte;
  }
  else
  {
    return sourceByte;
  }
}

uint8_t Core::fetchROM( uint16_t address )
{
  uint8_t sourceByte = mROM[address];
  if constexpr ( ENABLE_TRAPS )
  {
    uint8_t filteredByte = mScriptDebugger->executeROM( *this, address, sourceByte );
    return filteredByte;
  }
  else
  {
    return sourceByte;
  }
}

uint8_t Core::readRAM( uint16_t address )
{
  uint8_t sourceByte = mRAM[address];
  if constexpr ( ENABLE_TRAPS )
  {
    uint8_t filteredByte = mScriptDebugger->readRAM( *this, address, sourceByte );
    return filteredByte;
  }
  else
  {
    return sourceByte;
  }
}

uint8_t Core::readROM( uint16_t address )
{
  uint8_t sourceByte = mROM[address];
  if constexpr ( ENABLE_TRAPS )
  {
    uint8_t filteredByte = mScriptDebugger->readROM( *this, address, sourceByte );
    return filteredByte;
  }
  else
  {
    return sourceByte;
  }
}

void Core::writeRAM( uint16_t address, uint8_t value )
{
  if constexpr ( ENABLE_TRAPS )
  {
    uint8_t filteredByte = mScriptDebugger->writeRAM( *this, address, value );
    mRAM[address] = filteredByte;
  }
  else
  {
    mRAM[address] = value;
  }
}

uint8_t Core::readMikey( uint16_t address )
{
  mCurrentTick = mMikey->requestAccess( mCurrentTick, address );
  uint8_t sourceByte = mMikey->read( address );
  if constexpr ( ENABLE_TRAPS )
  {
    uint8_t filteredByte = mScriptDebugger->readMikey( *this, address, sourceByte );
    return filteredByte;
  }
  else
  {
    return sourceByte;
  }
}

void Core::writeMikey( uint16_t address, uint8_t value )
{
  if constexpr ( ENABLE_TRAPS )
  {
    uint8_t filteredByte = mScriptDebugger->writeMikey( *this, address, value );
    if ( auto mikeyAction = mMikey->write( address, filteredByte ) )
    {
      mActionQueue.push( mikeyAction );
    }
  }
  else
  {
    if ( auto mikeyAction = mMikey->write( address, value ) )
    {
      mActionQueue.push( mikeyAction );
    }
  }
}

uint8_t Core::readSuzy( uint16_t address )
{
  uint8_t sourceByte = mSuzy->read( address );
  if constexpr ( ENABLE_TRAPS )
  {
    uint8_t filteredByte = mScriptDebugger->readSuzy( *this, address, sourceByte );
    return filteredByte;
  }
  else
  {
    return sourceByte;
  }
}

void Core::writeSuzy( uint16_t address, uint8_t value )
{
  if constexpr ( ENABLE_TRAPS )
  {
    uint8_t filteredByte = mScriptDebugger->writeSuzy( *this, address, value );
    mSuzy->write( address, filteredByte );
  }
  else
  {
    mSuzy->write( address, value );
  }
}

uint8_t Core::readROM( uint16_t address, bool isFetch )
{
  if ( address >= 0x1fa )
  {
    if ( mMapCtl.vectorSpaceDisable )
    {
      return isFetch ? fetchRAM( address + 0xfe00 ) : readRAM( address + 0xfe00 );
    }
    else
    {
      return isFetch ? fetchROM( address ) : readROM( address );
    }
  }
  else if ( address < 0x1f8 )
  {
    if ( mMapCtl.romDisable )
    {
      return isFetch ? fetchRAM( address + 0xfe00 ) : readRAM( address + 0xfe00 );
    }
    else
    {
      return isFetch ? fetchROM( address ) : readROM( address );
    }
  }
  else if ( address == 0x1f9 )
  {
    uint8_t result = 0xf0 | //high nibble of MAPCTL is set
      ( mMapCtl.vectorSpaceDisable ? 0x08 : 0x00 ) |
      ( mMapCtl.romDisable ? 0x04 : 0x00 ) |
      ( mMapCtl.mikeyDisable ? 0x02 : 0x00 ) |
      ( mMapCtl.suzyDisable ? 0x01 : 0x00 );

    return mScriptDebugger->readMapCtl( *this, result );
  }
  else
  {
    //there is always RAM at 0xfff8
    return isFetch ? fetchRAM( address + 0xfe00 ) : readRAM( address + 0xfe00 );
  }
}

void Core::writeROM( uint16_t address, uint8_t value )
{
  if ( address >= 0x1fa && mMapCtl.vectorSpaceDisable || address < 0x1f8 && mMapCtl.romDisable || address == 0x1f8 )
  {
    writeRAM( 0xfe00 + address, value );
  }
  else if ( address == 0x1f9 )
  {
    value = mScriptDebugger->writeMapCtl( *this, value );
    writeMAPCTL( value );
  }
  else
  {
    mScriptDebugger->writeROM( *this, address, value );
    //ignore write to ROM
  }
}

void Core::writeMAPCTL( uint8_t value )
{
  value = mScriptDebugger->writeMapCtl( *this, value );

  mMapCtl.sequentialDisable = ( value & 0x80 ) != 0;
  mMapCtl.vectorSpaceDisable = ( value & 0x08 ) != 0;
  mMapCtl.romDisable = ( value & 0x04 ) != 0;
  mMapCtl.mikeyDisable = ( value & 0x02 ) != 0;
  mMapCtl.suzyDisable = ( value & 0x01 ) != 0;

  mFastCycleTick = mMapCtl.sequentialDisable ? 5 : 4;
  mPageTypes[0xfe] = mMapCtl.romDisable ? PageType::RAM : PageType::ROM;
  mPageTypes[0xfd] = mMapCtl.mikeyDisable ? PageType::RAM : PageType::MIKEY;
  mPageTypes[0xfc] = mMapCtl.suzyDisable ? PageType::RAM : PageType::SUZY;
}

uint64_t Core::tick() const
{
  return mCurrentTick;
}

uint8_t Core::debugReadRAM( uint16_t address ) const
{
  return mRAM[address];
}

uint8_t Core::debugReadROM( uint16_t address ) const
{
  return mROM[address & 0x1ff];
}

void Core::debugWriteRAM( uint16_t address, uint8_t value )
{
  mRAM[address] = value;
}

uint8_t Core::debugReadMikey( uint16_t address ) const
{
  mMikey->requestAccess( mCurrentTick, address );
  return mMikey->read( address );
}

void Core::debugWriteMikey( uint16_t address, uint8_t value )
{
  mMikey->requestAccess( mCurrentTick, address );
  if ( auto mikeyAction = mMikey->write( address, value ) )
  {
    mActionQueue.push( mikeyAction );
  }
}

uint8_t Core::debugReadSuzy( uint16_t address ) const
{
  mSuzy->requestRead( mCurrentTick, address );
  return mSuzy->read( address );
}

void Core::debugWriteSuzy( uint16_t address, uint8_t value )
{
  mSuzy->requestWrite( mCurrentTick, address );
  mSuzy->write( address, value );
}

CPUState& Core::debugState()
{
  return mCpu->state();
}

CPU& Core::debugCPU()
{
  return *mCpu;
}

uint8_t const* Core::debugRAM()
{
  return &mRAM[0];
}
uint8_t const* Core::debugROM()
{
  return &mROM[0];
}

uint16_t Core::debugDispAdr() const
{
  return mMikey->debugDispAdr();
}

uint16_t Core::debugVidBas() const
{
  return mSuzy->debugVidBas();
}

uint16_t Core::debugCollBas() const
{
  return mSuzy->debugCollBas();
}

std::span<uint8_t const, 32> Core::debugPalette() const
{
  return mMikey->debugPalette();
}


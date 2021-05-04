#include "pch.hpp"
#include "Felix.hpp"
#include "CPU.hpp"
#include "Cartridge.hpp"
#include "ComLynx.hpp"
#include "Mikey.hpp"
#include "InputFile.hpp"
#include "ImageBS93.hpp"
#include "ImageBIOS.hpp"
#include "ImageCart.hpp"
#include "ImageBIN.hpp"

static constexpr uint32_t BAD_SEQ_ACCESS_ADDRESS = 0xbadc0ffeu;

Felix::Felix( std::function<void( DisplayGenerator::Pixel const* )> const& fun, std::function<KeyInput()> const& inputProvider ) : mRAM{}, mROM{}, mPageTypes{}, mCurrentTick{}, mSamplesRemainder{}, mActionQueue{},
  mCpu{ std::make_shared<CPU>( *this, false ) }, mCartridge{ std::make_shared<Cartridge>( std::make_shared<ImageCart>() ) }, mComLynx{ std::make_shared<ComLynx>() }, mMikey{ std::make_shared<Mikey>( *this, fun ) }, mSuzy{ std::make_shared<Suzy>( *this, inputProvider ) },
  mMapCtl{}, mSequencedAccessAddress{ BAD_SEQ_ACCESS_ADDRESS }, mDMAAddress{}, mFastCycleTick{ 4 }, mResetRequestDuringSpriteRendering{}, mSuzyRunning{}
{
  //for ( auto it = mRAM.begin(); it != mRAM.end(); ++it )
  //{
  //  *it = (uint8_t)rand();
  //}

  for ( size_t i = 0; i < mPageTypes.size(); ++i )
  {
    switch ( i )
    {
      case 0xff:
        mPageTypes[i] = PageType::FF;
        break;
      case 0xfe:
        mPageTypes[i] = PageType::FE;
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

  mCpu->res().target();
}

void Felix::injectFile( InputFile const & file )
{
  switch ( file.getType() )
  {
  case InputFile::FileType::BS93:
    if ( auto runAddress = file.getBS93()->load( mRAM.data() ) )
    {
      pulseReset( runAddress );
    }
    break;
  case InputFile::FileType::BIOS:
    file.getBIOS()->load( mROM.data() );
    pulseReset();
    break;
  case InputFile::FileType::CART:
    mCartridge = std::make_shared<Cartridge>( file.getCart() );
    pulseReset();
    break;
  case InputFile::FileType::BIN:
    file.getBIN()->load( mRAM.data() );
    pulseReset( 0x400 );
    break;
  default:
    break;
  }
}

void Felix::pulseReset( std::optional<uint16_t> resetAddress )
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
    assertInterrupt( CPU::I_RESET, mCurrentTick );
    desertInterrupt( CPU::I_RESET, mCurrentTick + 5 * 10 );  //asserting RESET for 10 cycles to make sure none will miss it
  }
}

Felix::~Felix()
{
}

void Felix::requestDisplayDMA( uint64_t tick, uint16_t address )
{
  mDMAAddress = address;
  mActionQueue.push( { Action::DISPLAY_DMA, tick } );
}

void Felix::runSuzy()
{
  mSuzyRunning = true;
  if ( !mSuzyProcess )
    mSuzyProcess = mSuzy->suzyProcess();
}

void Felix::assertInterrupt( int mask, std::optional<uint64_t> tick )
{
  if ( ( mask & CPU::I_IRQ ) != 0 )
  {
    mActionQueue.push( { Action::ASSERT_IRQ, tick.value_or( mCurrentTick ) } );
    return;
  }
  else if ( ( mask & CPU::I_RESET ) != 0 )
  {
    mActionQueue.push( { Action::ASSERT_RESET, tick.value_or( mCurrentTick ) } );
    return;
  }
  else
  {
    assert( !"Unsupported interrupt" );
  }
}

void Felix::desertInterrupt( int mask, std::optional<uint64_t> tick )
{
  if ( ( mask & CPU::I_IRQ ) != 0 )
  {
    mActionQueue.push( { Action::DESERT_IRQ, tick.value_or( mCurrentTick ) } );
    return;
  }
  else if ( ( mask & CPU::I_RESET ) != 0 )
  {
    mActionQueue.push( { Action::DESERT_RESET, tick.value_or( mCurrentTick ) } );
    return;
  }
  else
  {
    assert( !"Unsupported interrupt" );
  }
}

bool Felix::executeSequencedAction( SequencedAction seqAction )
{
  mCurrentTick = seqAction.getTick();
  auto action = seqAction.getAction();

  auto & res = mCpu->res();

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
    if ( auto newAction = mMikey->fireTimer( mCurrentTick, (int)action - (int)Action::FIRE_TIMER0 ) )
    {
      mActionQueue.push( newAction );
    }
    break;
  case Action::ASSERT_IRQ:
    res.interrupt |= CPU::I_IRQ;
    break;
  case Action::ASSERT_RESET:
    res.interrupt |= CPU::I_RESET;
    break;
  case Action::DESERT_IRQ:
    res.interrupt &= ~CPU::I_IRQ;
    break;
  case Action::DESERT_RESET:
    res.interrupt &= ~CPU::I_RESET;
    break;
  case Action::END_FRAME:
    return true;
  default:
    assert( false );
    break;
  }
  return false;
}

bool Felix::executeSuzyAction()
{
  if ( !mSuzyProcess || !mSuzyRunning )
    return true;

  auto & res = mCpu->res();

  if ( res.interrupt != 0 )
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
    mSuzyProcess->respond( mRAM[mSuzyProcessRequest->addr] );
    mCurrentTick += 5ull; //read byte
    break;
  case ISuzyProcess::Request::READ4:
    if ( mSuzyProcessRequest->addr <= 0xfffc )
      mSuzyProcess->respond( *( (uint32_t const *)( mRAM.data() + mSuzyProcessRequest->addr ) ) );
    mCurrentTick += 5ull + 3 * mFastCycleTick;  //read 4 bytes
    break;
  case ISuzyProcess::Request::WRITE:
    mRAM[mSuzyProcessRequest->addr] = mSuzyProcessRequest->value;
    mCurrentTick += 5ull; //write byte
    break;
  case ISuzyProcess::Request::COLRMW:
    {
      assert( mSuzyProcessRequest->addr <= 0xfffc );
      const uint32_t value = *( (uint32_t const *)( mRAM.data() + mSuzyProcessRequest->addr ) );

      //broadcast
      const uint8_t u8 = mSuzyProcessRequest->value;
      const uint16_t u16 = mSuzyProcessRequest->value | ( mSuzyProcessRequest->value << 8 );
      const uint32_t u32 = u16 | ( u16 << 16 );

      const uint32_t rmwvalue = ( value & ~mSuzyProcessRequest->mask ) | ( u32 & mSuzyProcessRequest->mask );
      *( (uint32_t *)( mRAM.data() + mSuzyProcessRequest->addr ) ) = rmwvalue;

      const uint32_t resvalue = value & mSuzyProcessRequest->mask;
      uint8_t result{};

      //horizontal max
      for ( int i = 0; i < 8; ++i )
      {
        result = std::max( result, (uint8_t)( resvalue >> ( i * 4 ) & 0x0f ) );
      }
      mSuzyProcess->respond( result );
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

void Felix::executeCPUAction()
{
  auto & req = mCpu->req();
  auto & res = mCpu->res();

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
    FETCH_OPCODE_FE,
    FETCH_OPERAND_FE,
    READ_FE,
    WRITE_FE,
    FETCH_OPCODE_FF,
    FETCH_OPERAND_FF,
    READ_FF,
    WRITE_FF
  };
  
  CPUAction action = (CPUAction)( (int)req.type + (int)pageType );

  switch ( action )
  {
  case CPUAction::FETCH_OPCODE_RAM:
    [[fallthrough]];
  case CPUAction::FETCH_OPERAND_RAM:
    [[fallthrough]];
  case CPUAction::READ_RAM:
    res.value = mRAM[req.address];
    res.tick = mCurrentTick;
    mCurrentTick += ( req.address == mSequencedAccessAddress ) ? mFastCycleTick : 5;
    mSequencedAccessAddress = req.address + 1;
    res.target();
    break;
  case CPUAction::WRITE_RAM:
    mRAM[req.address] = req.value;
    mCurrentTick += 5;
    mSequencedAccessAddress = BAD_SEQ_ACCESS_ADDRESS;
    res.target();
    break;
  case CPUAction::FETCH_OPCODE_FE:
    [[fallthrough]];
  case CPUAction::FETCH_OPERAND_FE:
    [[fallthrough]];
  case CPUAction::READ_FE:
    res.value = mROM[req.address & 0x1ff];
    mCurrentTick += ( req.address == mSequencedAccessAddress ) ? mFastCycleTick : 5;
    mSequencedAccessAddress = req.address + 1;
    res.target();
    break;
  case CPUAction::WRITE_FE:
    mCurrentTick += 5;
    mSequencedAccessAddress = BAD_SEQ_ACCESS_ADDRESS;
    res.target();
    break;
  case CPUAction::FETCH_OPCODE_FF:
    [[fallthrough]];
  case CPUAction::FETCH_OPERAND_FF:
    [[fallthrough]];
  case CPUAction::READ_FF:
    res.value = readFF( req.address & 0xff );
    mCurrentTick += ( req.address == mSequencedAccessAddress ) ? mFastCycleTick : 5;
    mSequencedAccessAddress = req.address + 1;
    res.target();
    break;
  case CPUAction::WRITE_FF:
    writeFF( req.address & 0xff, req.value );
    mCurrentTick += 5;
    mSequencedAccessAddress = BAD_SEQ_ACCESS_ADDRESS;
    res.target();
    break;
  case CPUAction::FETCH_OPCODE_SUZY:
    [[fallthrough]];
  case CPUAction::FETCH_OPERAND_SUZY:
    assert( false );
    [[fallthrough]];
  case CPUAction::READ_SUZY:
    res.value = mSuzy->read( req.address );
    mCurrentTick = mSuzy->requestAccess( mCurrentTick, req.address );
    mSequencedAccessAddress = BAD_SEQ_ACCESS_ADDRESS;
    res.target();
    break;
  case CPUAction::WRITE_SUZY:
    mSuzy->write( req.address, req.value );
    mCurrentTick = mSuzy->requestAccess( mCurrentTick, req.address );
    mSequencedAccessAddress = BAD_SEQ_ACCESS_ADDRESS;
    res.target();
    break;
  case CPUAction::FETCH_OPCODE_MIKEY:
    [[fallthrough]];
  case CPUAction::FETCH_OPERAND_MIKEY:
    assert( false );
    [[fallthrough]];
  case CPUAction::READ_MIKEY:
    res.value = mMikey->read( req.address );
    mCurrentTick = mMikey->requestAccess( mCurrentTick, req.address );
    mSequencedAccessAddress = BAD_SEQ_ACCESS_ADDRESS;
    res.target();
    break;
  case CPUAction::WRITE_MIKEY:
    if ( auto mikeyAction = mMikey->write( req.address, req.value ) )
    {
      mActionQueue.push( mikeyAction );
    }
    mCurrentTick = mSuzy->requestAccess( mCurrentTick, req.address );
    mSequencedAccessAddress = BAD_SEQ_ACCESS_ADDRESS;
    res.target();
    break;
  }
}

void Felix::process( uint64_t ticks )
{
  mActionQueue.push( { Action::END_FRAME, mCurrentTick + ticks } );

  auto & req = mCpu->req();

  for ( ;; )
  {
    if ( mActionQueue.head().getTick() <= mCurrentTick )
    {
      if ( executeSequencedAction( mActionQueue.pop() ) )
        return;
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

std::pair<float, float> Felix::getSample( int sps )
{
  int cnt = 16000000 / sps;
  mSamplesRemainder += 16000000 % sps;
  if ( mSamplesRemainder > sps )
  {
    mSamplesRemainder &= sps;
    cnt += 1;
  }

  process( cnt );

  return mMikey->sampleAudio();
}

void Felix::enterMonitor()
{
}

Cartridge & Felix::getCartridge()
{
  assert( mCartridge );
  return *mCartridge;
}

ComLynx & Felix::getComLynx()
{
  assert( mComLynx );
  return *mComLynx;
}

uint8_t Felix::readFF( uint16_t address )
{
  if ( address >= 0xfa )
  {
    uint8_t * ptr = mMapCtl.vectorSpaceDisable ? ( mRAM.data() + 0xff00 ) : ( mROM.data() + 0x100 );
    return ptr[address];
  }
  else if ( address < 0xf8 )
  {
    uint8_t * ptr = mMapCtl.kernelDisable ? ( mRAM.data() + 0xff00 ) : ( mROM.data() + 0x100 );
    return ptr[address];
  }
  else if ( address == 0xf9 )
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
    return mRAM[0xff00 + address];
  }
}

void Felix::writeFF( uint16_t address, uint8_t value )
{
  if ( address >= 0xfa && mMapCtl.vectorSpaceDisable || address < 0xf8 && mMapCtl.kernelDisable || address == 0xf8 )
  {
    mRAM[0xff00 + address] = value;
  }
  else if ( address == 0xf9 )
  {
    writeMAPCTL( value );
  }
  else
  {
    //ignore write to ROM
  }
}

void Felix::writeMAPCTL( uint8_t value )
{
  mMapCtl.sequentialDisable = ( value & 0x80 ) != 0;
  mMapCtl.vectorSpaceDisable = ( value & 0x08 ) != 0;
  mMapCtl.kernelDisable = ( value & 0x04 ) != 0;
  mMapCtl.mikeyDisable = ( value & 0x02 ) != 0;
  mMapCtl.suzyDisable = ( value & 0x01 ) != 0;

  mFastCycleTick = mMapCtl.sequentialDisable ? 5 : 4;
  mPageTypes[0xfe] = mMapCtl.kernelDisable ? PageType::RAM : PageType::FE;
  mPageTypes[0xfd] = mMapCtl.mikeyDisable ? PageType::RAM : PageType::MIKEY;
  mPageTypes[0xfc] = mMapCtl.suzyDisable ? PageType::RAM : PageType::SUZY;
}

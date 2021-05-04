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

Felix::Felix( std::function<void( DisplayGenerator::Pixel const* )> const& fun, std::function<KeyInput()> const& inputProvider ) : mRAM{}, mROM{}, mPageTypes{}, mBusReservationTick{}, mCurrentTick{}, mSamplesRemainder{}, mActionQueue{},
mCpu{ std::make_shared<CPU>( *this, false ) }, mCartridge{ std::make_shared<Cartridge>( std::make_shared<ImageCart>() ) }, mComLynx{ std::make_shared<ComLynx>() }, mMikey{ std::make_shared<Mikey>( *this, fun ) }, mSuzy{ std::make_shared<Suzy>( *this, inputProvider ) },
  mMapCtl{}, mSequencedAccessAddress{ ~0u }, mDMAAddress{}, mFastCycleTick{ 4 }, mResetRequestDuringSpriteRendering{}
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

void Felix::process( uint64_t ticks )
{
  mActionQueue.push( { Action::END_FRAME, mCurrentTick + ticks } );

  auto & req = mCpu->req();
  auto & res = mCpu->res();

  for ( ;; )
  {
    auto seqAction = mActionQueue.pop();
    mCurrentTick = seqAction.getTick();
    auto action = seqAction.getAction();

    switch ( action )
    {
      case Action::DISPLAY_DMA:
        mMikey->setDMAData( mCurrentTick, *(uint64_t*)( mRAM.data() + mDMAAddress ) );
        mBusReservationTick += 6 * mFastCycleTick + 2 * 5;
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
      if ( auto newAction = mMikey->fireTimer( mCurrentTick, ( int )action - ( int )Action::FIRE_TIMER0 ) )
      {
        mActionQueue.push( newAction );
      }
      break;
    case Action::CPU_FETCH_OPCODE_RAM:
      res.value = mRAM[req.address];
      mSequencedAccessAddress = req.address + 1;
      res.tick = mCurrentTick;
      res.target();
      break;
    case Action::CPU_FETCH_OPERAND_RAM:
      res.value = mRAM[req.address];
      mSequencedAccessAddress = req.address + 1;
      res.target();
      break;
    case Action::CPU_READ_RAM:
      res.value = mRAM[req.address];
      mSequencedAccessAddress = req.address + 1;
      res.target();
      break;
    case Action::CPU_WRITE_RAM:
      mRAM[req.address] = req.value;
      mSequencedAccessAddress = ~0;
      res.target();
      break;
    case Action::CPU_FETCH_OPCODE_FE:
      res.value = mROM[req.address & 0x1ff];
      mSequencedAccessAddress = req.address + 1;
      res.tick = mCurrentTick;
      res.target();
      break;
    case Action::CPU_FETCH_OPERAND_FE:
      res.value = mROM[req.address & 0x1ff];
      mSequencedAccessAddress = req.address + 1;
      res.target();
      break;
    case Action::CPU_READ_FE:
      res.value = mROM[req.address & 0x1ff];
      mSequencedAccessAddress = req.address + 1;
      res.target();
      break;
    case Action::CPU_WRITE_FE:
      mSequencedAccessAddress = ~0;
      res.target();
      break;
    case Action::CPU_FETCH_OPCODE_FF:
      res.value = readFF( req.address & 0xff );
      mSequencedAccessAddress = req.address + 1;
      res.tick = mCurrentTick;
      res.target();
      break;
    case Action::CPU_FETCH_OPERAND_FF:
      res.value = readFF( req.address & 0xff );
      mSequencedAccessAddress = req.address + 1;
      res.target();
      break;
    case Action::CPU_READ_FF:
      res.value = readFF( req.address & 0xff );
      mSequencedAccessAddress = req.address + 1;
      res.target();
      break;
    case Action::CPU_WRITE_FF:
      writeFF( req.address & 0xff, req.value );
      mSequencedAccessAddress = ~0;
      res.target();
      break;
    case Action::CPU_READ_SUZY:
      res.value = mSuzy->read( req.address );
      res.target();
      break;
    case Action::CPU_WRITE_SUZY:
      mSuzy->write( req.address, req.value );
      res.target();
      break;
    case Action::CPU_READ_MIKEY:
      res.value = mMikey->read( req.address );
      res.target();
      break;
    case Action::CPU_WRITE_MIKEY:
      switch ( auto mikeyAction = mMikey->write( req.address, req.value ) )
      {
      case Mikey::WriteAction::Type::START_SUZY:
        if ( !mSuzyProcess )
        {
          mSuzyProcess = mSuzy->suzyProcess();
        }
        processSuzy();
        break;
      case Mikey::WriteAction::Type::ENQUEUE_ACTION:
        mActionQueue.push( mikeyAction.action );
        [[fallthrough]];
      case Mikey::WriteAction::Type::NONE:
        res.target();
        break;
      }
      break;
    case Action::SUZY_NONE:
      mSuzyProcess.reset();
      //workaround to problem with resetting during Suzy activity
      if ( mResetRequestDuringSpriteRendering )
      {
        mResetRequestDuringSpriteRendering = false;
        pulseReset();
      }
      res.target();
      break;
    case Action::SUZY_READ:
      suzyRead();
      processSuzy();
      break;
    case Action::SUZY_READ4:
      suzyRead4();
      processSuzy();
      break;
    case Action::SUZY_WRITE:
      suzyWrite();
      processSuzy();
      break;
    case Action::SUZY_COLRMW:
      suzyColRMW();
      processSuzy();
      break;
    case Action::SUZY_VIDRMW:
      suzyVidRMW();
      processSuzy();
      break;
    case Action::SUZY_XOR:
      suzyXor();
      processSuzy();
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
      return;
    case Action::CPU_FETCH_OPCODE_SUZY:
    case Action::CPU_FETCH_OPCODE_MIKEY:
    case Action::CPU_FETCH_OPERAND_SUZY:
    case Action::CPU_FETCH_OPERAND_MIKEY:
      assert( false );
      break;
    default:
      break;
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

void Felix::suzyRead()
{
  auto value = mRAM[mSuzyProcessRequest->addr];
  mSuzyProcess->respond( value );
}

void Felix::suzyRead4()
{
  assert( mSuzyProcessRequest->addr <= 0xfffc );
  auto value = *( ( uint32_t const* )( mRAM.data() + mSuzyProcessRequest->addr ) );
  mSuzyProcess->respond( value );
}

void Felix::suzyWrite()
{
  mRAM[mSuzyProcessRequest->addr] = mSuzyProcessRequest->value;
}

void Felix::suzyColRMW()
{
  assert( mSuzyProcessRequest->addr <= 0xfffc );
  const uint32_t value = *( (uint32_t const*)( mRAM.data() + mSuzyProcessRequest->addr ) );

  //broadcast
  const uint8_t u8 = mSuzyProcessRequest->value;
  const uint16_t u16 = mSuzyProcessRequest->value | ( mSuzyProcessRequest->value << 8 );
  const uint32_t u32 = u16 | ( u16 << 16 );

  const uint32_t rmwvaleu = ( value & ~mSuzyProcessRequest->mask ) | ( u32 & mSuzyProcessRequest->mask );
  *( (uint32_t*)( mRAM.data() + mSuzyProcessRequest->addr ) ) =  rmwvaleu;

  const uint32_t resvalue = value & mSuzyProcessRequest->mask;
  uint8_t result{};

  //horizontal max
  for ( int i = 0; i < 8; ++i )
  {
    result = std::max( result, (uint8_t)( resvalue >> ( i * 4 ) & 0x0f ) );
  }
  mSuzyProcess->respond( result );
}

void Felix::suzyVidRMW()
{
  auto value = mRAM[mSuzyProcessRequest->addr] & mSuzyProcessRequest->mask | mSuzyProcessRequest->value;
  mRAM[mSuzyProcessRequest->addr] = ( uint8_t )value;
}

void Felix::suzyXor()
{
  auto value = mRAM[mSuzyProcessRequest->addr] ^ mSuzyProcessRequest->value;
  mRAM[mSuzyProcessRequest->addr] = ( uint8_t )value;
}

void Felix::processSuzy()
{
  static constexpr std::array<int, ( int )ISuzyProcess::Request::Type::_SIZE> requestCost ={
    0, //NONE,
    0, //READ,
    3, //READ4,
    0, //WRITE,
    7, //COLRMW,
    1, //VIDRMW,
    1  //XOR,
  };

  auto & res = mCpu->res();

  if ( res.interrupt != 0 )
  {
    res.target();
    return;
  }

  mSuzyProcessRequest = mSuzyProcess->advance();
  int op = ( int )mSuzyProcessRequest->type;
  mActionQueue.push( { ( Action )( ( int )Action::SUZY_NONE + op ), mBusReservationTick } );
  if ( op )
  {
    mBusReservationTick += 5ull + requestCost[op] * mFastCycleTick;
  }
}

void Felix::processCPU()
{
  static constexpr std::array<Action, 25> requestToAction = {
    Action::NONE,
    Action::CPU_FETCH_OPCODE_RAM,
    Action::CPU_FETCH_OPERAND_RAM,
    Action::CPU_READ_RAM,
    Action::CPU_WRITE_RAM,
    Action::NONE_FE,
    Action::CPU_FETCH_OPCODE_FE,
    Action::CPU_FETCH_OPERAND_FE,
    Action::CPU_READ_FE,
    Action::CPU_WRITE_FE,
    Action::NONE_FF,
    Action::CPU_FETCH_OPCODE_FF,
    Action::CPU_FETCH_OPERAND_FF,
    Action::CPU_READ_FF,
    Action::CPU_WRITE_FF,
    Action::NONE_MIKEY,
    Action::CPU_FETCH_OPCODE_MIKEY,
    Action::CPU_FETCH_OPERAND_MIKEY,
    Action::CPU_READ_MIKEY,
    Action::CPU_WRITE_MIKEY,
    Action::NONE_SUZY,
    Action::CPU_FETCH_OPCODE_SUZY,
    Action::CPU_FETCH_OPERAND_SUZY,
    Action::CPU_READ_SUZY,
    Action::CPU_WRITE_SUZY
  };

  auto & req = mCpu->req();

  auto pageType = mPageTypes[req.address >> 8];
  mActionQueue.push( { requestToAction[( size_t )req.type + ( size_t )pageType], mBusReservationTick } );
  switch ( pageType )
  {
    case PageType::RAM:
    case PageType::FE:
    case PageType::FF:
      mBusReservationTick += ( req.address == mSequencedAccessAddress ) ? mFastCycleTick : 5;
      break;
    case PageType::MIKEY:
      mBusReservationTick = mMikey->requestAccess( mBusReservationTick, req.address );
      break;
    case PageType::SUZY:
      mBusReservationTick = mSuzy->requestAccess( mBusReservationTick, req.address );
      break;
  }
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

#include "BusMaster.hpp"
#include "CPU.hpp"
#include "Suzy.hpp"
#include "Mikey.hpp"
#include <fstream>
#include <filesystem>
#include <cassert>

BusMaster::BusMaster() : mRAM{}, mROM{}, mPageTypes{}, mBusReservationTick{}, mCurrentTick{},
  mCpu{ std::make_shared<CPU>() }, mMikey{ std::make_shared<Mikey>( *this ) }, mSuzy{ std::make_shared<Suzy>() }, mCpuExecute{ cpuExecute( *mCpu ) }, mSuzyExecute{}, mActionQueue{}, mCPUReq{}, mMapCtl{}, mSequencedAccessAddress{ ~0u }, mDMAAddress{}, mFastCycleTick{ 4 }
{
  {
    std::ifstream fin{ "lynxboot.img", std::ios::binary };
    if ( fin.bad() )
      throw std::exception{};

    fin.read( ( char* )mROM.data(), mROM.size() );
  }
  if ( size_t size = (size_t)std::filesystem::file_size( "d:/tests/5.o" ) )
  {
    std::ifstream fin{ "d:/tests/5.o", std::ios::binary };
    if ( fin.bad() )
      throw std::exception{};

    fin.read( ( char* )mRAM.data() + 0x1f6, size );

    mROM[0x1fc] = 0x00;
    mROM[0x1fd] = 0x02;
  }

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

  mCpuExecute.setBusMaster( this );
}

BusMaster::~BusMaster()
{
}

CPURequest * BusMaster::request( CPURead r )
{
  mCPUReq = CPURequest{ r };
  request( mCPUReq );
  return &mCPUReq;
}

CPURequest * BusMaster::request( CPUFetchOpcode r )
{
  mCPUReq = CPURequest{ r };
  request( mCPUReq );
  return &mCPUReq;
}

CPURequest * BusMaster::request( CPUFetchOperand r )
{
  mCPUReq = CPURequest{ r };
  request( mCPUReq );
  return &mCPUReq;
}

CPURequest * BusMaster::request( CPUWrite w )
{
  mCPUReq = CPURequest{ w };
  request( mCPUReq );
  return &mCPUReq;
}

SuzyRequest * BusMaster::request( SuzyFetchSCB r )
{
  mSuzyReq = SuzyRequest{ r };
  request( mCPUReq );
  return &mSuzyReq;
}

SuzyRequest * BusMaster::request( SuzyFetchSprite r )
{
  mSuzyReq = SuzyRequest{ r };
  request( mCPUReq );
  return &mSuzyReq;
}

SuzyRequest * BusMaster::request( SuzyReadPixel r )
{
  mSuzyReq = SuzyRequest{ r };
  request( mCPUReq );
  return &mSuzyReq;
}

SuzyRequest * BusMaster::request( SuzyWritePixel w )
{
  mSuzyReq = SuzyRequest{ w };
  request( mCPUReq );
  return &mSuzyReq;
}

void BusMaster::requestDisplayDMA( uint64_t tick, uint16_t address )
{
  mDMAAddress = address;
  mActionQueue.push( { Action::DISPLAY_DMA, tick } );
}

TraceRequest & BusMaster::getTraceRequest()
{
  return mDReq;
}

DisplayGenerator::Pixel const* BusMaster::process( uint64_t ticks )
{
  mActionQueue.push( { Action::END_FRAME, mCurrentTick + ticks } );

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
      mCPUReq.value = mRAM[mCPUReq.address];
      mSequencedAccessAddress = mCPUReq.address + 1;
      mCurrentTick;
      mCPUReq.tick = mCurrentTick;
      mCPUReq.interrupt = mMikey->getIRQ() != 0 ? CPU::I_IRQ : 0;
      mCPUReq.resume();
      mDReq.resume();
      break;
    case Action::CPU_FETCH_OPERAND_RAM:
      mCPUReq.value = mRAM[mCPUReq.address];
      mSequencedAccessAddress = mCPUReq.address + 1;
      mCPUReq.resume();
      mDReq.resume();
      break;
    case Action::CPU_READ_RAM:
      mCPUReq.value = mRAM[mCPUReq.address];
      mSequencedAccessAddress = mCPUReq.address + 1;
      mCPUReq.resume();
      break;
    case Action::CPU_WRITE_RAM:
      mRAM[mCPUReq.address] = mCPUReq.value;
      mSequencedAccessAddress = ~0;
      mCPUReq.resume();
      break;
    case Action::CPU_FETCH_OPCODE_FE:
      mCPUReq.value = mROM[mCPUReq.address & 0x1ff];
      mSequencedAccessAddress = mCPUReq.address + 1;
      mCurrentTick;
      mCPUReq.tick = mCurrentTick;
      mCPUReq.interrupt = mMikey->getIRQ() != 0 ? CPU::I_IRQ : 0;
      mCPUReq.resume();
      mDReq.resume();
      break;
    case Action::CPU_FETCH_OPERAND_FE:
      mCPUReq.value = mROM[mCPUReq.address & 0x1ff];
      mSequencedAccessAddress = mCPUReq.address + 1;
      mCPUReq.resume();
      mDReq.resume();
      break;
    case Action::CPU_READ_FE:
      mCPUReq.value = mROM[mCPUReq.address & 0x1ff];
      mSequencedAccessAddress = mCPUReq.address + 1;
      mCPUReq.resume();
      break;
    case Action::CPU_WRITE_FE:
      mSequencedAccessAddress = ~0;
      mCPUReq.resume();
      break;
    case Action::CPU_FETCH_OPCODE_FF:
      mCPUReq.value = readFF( mCPUReq.address & 0xff );
      mSequencedAccessAddress = mCPUReq.address + 1;
      mCurrentTick;
      mCPUReq.tick = mCurrentTick;
      mCPUReq.interrupt = mMikey->getIRQ() != 0 ? CPU::I_IRQ : 0;
      mCPUReq.resume();
      mDReq.resume();
      break;
    case Action::CPU_FETCH_OPERAND_FF:
      mCPUReq.value = readFF( mCPUReq.address & 0xff );
      mSequencedAccessAddress = mCPUReq.address + 1;
      mCPUReq.resume();
      mDReq.resume();
      break;
    case Action::CPU_READ_FF:
      mCPUReq.value = readFF( mCPUReq.address & 0xff );
      mSequencedAccessAddress = mCPUReq.address + 1;
      mCPUReq.resume();
      break;
    case Action::CPU_WRITE_FF:
      writeFF( mCPUReq.address & 0xff, mCPUReq.value );
      mSequencedAccessAddress = ~0;
      mCPUReq.resume();
      break;
    case Action::CPU_READ_SUZY:
      mCPUReq.value = mSuzy->read( mCPUReq.address );
      mCPUReq.resume();
      break;
    case Action::CPU_WRITE_SUZY:
      if ( auto newAction = mSuzy->write( mCPUReq.address, mCPUReq.value ) )
      {
        mActionQueue.push( newAction );
      }
      mCPUReq.resume();
      break;
    case Action::CPU_READ_MIKEY:
      mCPUReq.value = mMikey->read( mCPUReq.address );
      mCPUReq.resume();
      break;
    case Action::CPU_WRITE_MIKEY:
      switch ( auto mikeyAction = mMikey->write( mCPUReq.address, mCPUReq.value ) )
      {
      case Mikey::WriteAction::Type::START_SUZY:
        mSuzyExecute = suzyExecute( *mSuzy );
        mSuzyExecute.setBusMaster( this );
        break;
      case Mikey::WriteAction::Type::FIRE_TIMER:
        mActionQueue.push( mikeyAction.action );
      case Mikey::WriteAction::Type::NONE:
        mCPUReq.resume();
        break;
      }
      break;
    case Action::END_FRAME:
      return mMikey->getSrface();
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

void BusMaster::request( CPURequest const & request )
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

  auto pageType = mPageTypes[request.address >> 8];
  mActionQueue.push( { requestToAction[( size_t )request.mType + ( int )pageType], mBusReservationTick } );
  switch ( pageType )
  {
    case PageType::RAM:
    case PageType::FE:
    case PageType::FF:
      mBusReservationTick += ( request.address == mSequencedAccessAddress ) ? mFastCycleTick : 5;
      break;
    case PageType::MIKEY:
      mBusReservationTick = mMikey->requestAccess( mBusReservationTick, request.address );
      break;
    case PageType::SUZY:
      mBusReservationTick = mSuzy->requestAccess( mBusReservationTick, request.address );
      break;
  }
}

void BusMaster::request( SuzyRequest const & request )
{
}

uint8_t BusMaster::readFF( uint16_t address )
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

void BusMaster::writeFF( uint16_t address, uint8_t value )
{
  if ( address >= 0xfa && mMapCtl.vectorSpaceDisable || address < 0xf8 && mMapCtl.kernelDisable || address == 0xf8 )
  {
    mRAM[0xff00 + address] = value;
  }
  else
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
}

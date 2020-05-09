#include "BusMaster.hpp"
#include "CPU.hpp"
#include "Suzy.hpp"
#include "Mikey.hpp"
#include <fstream>
#include <filesystem>
#include <cassert>

BusMaster::BusMaster( Mikey & mikey ) : mMikey{ mikey }, mRAM{}, mROM{}, mPageTypes{}, mBusReservationTick{}, mCurrentTick{},
  mSuzy{ std::make_shared<Suzy>() }, mActionQueue{}, mReq{}, mSequencedAccessAddress{ ~0u }, mDMAAddress{}
{
  mMikey.setDMARequestCallback( [this]( uint64_t tick, uint16_t address )
  {
    mDMAAddress = address;
    mActionQueue.push( { Action::DISPLAY_DMA, tick } );
  } );

  {
    std::ifstream fin{ "lynxboot.img", std::ios::binary };
    if ( fin.bad() )
      throw std::exception{};

    fin.read( ( char* )mROM.data(), mROM.size() );
  }
  if ( size_t size = (size_t)std::filesystem::file_size( "felix1.bin" ) )
  {
    std::ifstream fin{ "felix1.bin", std::ios::binary };
    if ( fin.bad() )
      throw std::exception{};

    fin.read( ( char* )mRAM.data() + 0x200, size );

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
}

BusMaster::~BusMaster()
{
}

CPURequest * BusMaster::request( CPURead r )
{
  mReq = CPURequest{ r };
  request( mReq );
  return &mReq;
}

CPURequest * BusMaster::request( CPUFetchOpcode r )
{
  mReq = CPURequest{ r };
  request( mReq );
  return &mReq;
}

CPURequest * BusMaster::request( CPUFetchOperand r )
{
  mReq = CPURequest{ r };
  request( mReq );
  return &mReq;
}

CPURequest * BusMaster::request( CPUWrite w )
{
  mReq = CPURequest{ w };
  request( mReq );
  return &mReq;
}

TraceRequest & BusMaster::getTraceRequest()
{
  return mDReq;
}

void BusMaster::process( uint64_t ticks )
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
        mMikey.setDMAData( mCurrentTick, *(uint64_t*)( mRAM.data() + mDMAAddress ) );
        mBusReservationTick += 6 * 4 + 2 * 5;
        break;
      case Action::FIRE_TIMER0:
      if ( auto newAction = mMikey.fireTimer( mCurrentTick, ( int )action - ( int )Action::FIRE_TIMER0 ) )
      {
        mActionQueue.push( newAction );
      }
      break;
    case Action::CPU_FETCH_OPCODE_RAM:
      mDReq.value = mReq.value = mRAM[mReq.address];
      mSequencedAccessAddress = mReq.address + 1;
      mDReq.cycle = mCurrentTick;
      //mDReq.interrupt = CPU::I_RESET;
      //mReq.interrupt = CPU::I_RESET;
      mDReq.resume();
      mReq.resume();
      break;
    case Action::CPU_FETCH_OPERAND_RAM:
      mDReq.value = mReq.value = mRAM[mReq.address];
      mSequencedAccessAddress = mReq.address + 1;
      mDReq.resume();
      mReq.resume();
      break;
    case Action::CPU_READ_RAM:
      mReq.value = mRAM[mReq.address];
      mSequencedAccessAddress = mReq.address + 1;
      mReq.resume();
      break;
    case Action::CPU_WRITE_RAM:
      mRAM[mReq.address] = mReq.value;
      mSequencedAccessAddress = ~0;
      mReq.resume();
      break;
    case Action::CPU_FETCH_OPCODE_FE:
      mDReq.value = mReq.value = mROM[mReq.address & 0x1ff];
      mSequencedAccessAddress = mReq.address + 1;
      mDReq.cycle = mCurrentTick;
      //mDReq.interrupt = CPU::I_RESET;
      //mReq.interrupt = CPU::I_RESET;
      mDReq.resume();
      mReq.resume();
      break;
    case Action::CPU_FETCH_OPERAND_FE:
      mDReq.value = mReq.value = mROM[mReq.address & 0x1ff];
      mSequencedAccessAddress = mReq.address + 1;
      mDReq.resume();
      mReq.resume();
      break;
    case Action::CPU_READ_FE:
      mReq.value = mROM[mReq.address & 0x1ff];
      mSequencedAccessAddress = mReq.address + 1;
      mReq.resume();
      break;
    case Action::CPU_WRITE_FE:
      mSequencedAccessAddress = ~0;
      mReq.resume();
      break;
    case Action::CPU_FETCH_OPCODE_FF:
      mDReq.value = mReq.value = readFF( mReq.address & 0xff );
      mSequencedAccessAddress = mReq.address + 1;
      mDReq.cycle = mCurrentTick;
      //mDReq.interrupt = CPU::I_RESET;
      //mReq.interrupt = CPU::I_RESET;
      mDReq.resume();
      mReq.resume();
      break;
    case Action::CPU_FETCH_OPERAND_FF:
      mDReq.value = mReq.value = readFF( mReq.address & 0xff );
      mSequencedAccessAddress = mReq.address + 1;
      mDReq.resume();
      mReq.resume();
      break;
    case Action::CPU_READ_FF:
      mReq.value = readFF( mReq.address & 0xff );
      mSequencedAccessAddress = mReq.address + 1;
      mReq.resume();
      break;
    case Action::CPU_WRITE_FF:
      writeFF( mReq.address & 0xff, mReq.value );
      mSequencedAccessAddress = ~0;
      mReq.resume();
      break;
    case Action::CPU_READ_SUZY:
      mReq.value = mSuzy->read( mReq.address );
      mReq.resume();
      break;
    case Action::CPU_WRITE_SUZY:
      if ( auto newAction = mSuzy->write( mReq.address, mReq.value ) )
      {
        mActionQueue.push( newAction );
      }
      mReq.resume();
      break;
    case Action::CPU_READ_MIKEY:
      mReq.value = mMikey.read( mReq.address );
      mReq.resume();
      break;
    case Action::CPU_WRITE_MIKEY:
      if ( auto newAction = mMikey.write( mReq.address, mReq.value ) )
      {
        mActionQueue.push( newAction );
      }
      mReq.resume();
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

  auto tick = mBusReservationTick;
  auto pageType = mPageTypes[request.address >> 8];
  mActionQueue.push( { requestToAction[( size_t )request.mType + ( int )pageType], mBusReservationTick } );
  switch ( pageType )
  {
    case PageType::RAM:
    case PageType::FE:
    case PageType::FF:
      mBusReservationTick += ( request.address == mSequencedAccessAddress ) ? 4 : 5;
      break;
    case PageType::MIKEY:
      mBusReservationTick = mMikey.requestAccess( mBusReservationTick, request.address );
      break;
    case PageType::SUZY:
      mBusReservationTick = mSuzy->requestAccess( mBusReservationTick, request.address );
      break;
  }
}

uint8_t BusMaster::readFF( uint16_t address )
{
  return *( mROM.data() + 0x100 + address );
}

void BusMaster::writeFF( uint16_t address, uint8_t value )
{
  *( mROM.data() + 0x100 + address ) = value;
}


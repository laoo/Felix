#include "BusMaster.hpp"
#include "CPU.hpp"
#include "Suzy.hpp"
#include "Mikey.hpp"
#include <fstream>
#include <filesystem>
#include <cassert>

BusMaster::BusMaster() : mRAM{}, mROM{}, mBusReservationTick{}, mCurrentTick{}, mMikey{ std::make_shared<Mikey>() }, mSuzy{ std::make_shared<Suzy>() }, mActionQueue{}, mReq{}, mSequencedAccessAddress{ ~0u }
{
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
    case Action::FIRE_TIMER0:
      if ( auto newAction = mMikey->fireTimer( mCurrentTick, ( int )action - 1 ) )
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
    case Action::CPU_FETCH_OPCODE_ROM:
      mDReq.value = mReq.value = mROM[mReq.address & 0x1ff];
      mSequencedAccessAddress = mReq.address + 1;
      mDReq.cycle = mCurrentTick;
      //mDReq.interrupt = CPU::I_RESET;
      //mReq.interrupt = CPU::I_RESET;
      mDReq.resume();
      mReq.resume();
      break;
    case Action::CPU_FETCH_OPERAND_ROM:
      mDReq.value = mReq.value = mROM[mReq.address & 0x1ff];
      mSequencedAccessAddress = mReq.address + 1;
      mDReq.resume();
      mReq.resume();
      break;
    case Action::CPU_READ_ROM:
      mReq.value = mROM[mReq.address & 0x1ff];
      mSequencedAccessAddress = mReq.address + 1;
      mReq.resume();
      break;
    case Action::CPU_WRITE_ROM:
      mROM[mReq.address & 0x1ff] = mReq.value;
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
      mReq.value = mMikey->read( mReq.address );
      mReq.resume();
      break;
    case Action::CPU_WRITE_MIKEY:
      if ( auto newAction = mMikey->write( mReq.address, mReq.value ) )
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
  static constexpr std::array<Action, 20> requestToAction = {
    Action::NONE,
    Action::CPU_FETCH_OPCODE_RAM,
    Action::CPU_FETCH_OPERAND_RAM,
    Action::CPU_READ_RAM,
    Action::CPU_WRITE_RAM,
    Action::NONE_ROM,
    Action::CPU_FETCH_OPCODE_ROM,
    Action::CPU_FETCH_OPERAND_ROM,
    Action::CPU_READ_ROM,
    Action::CPU_WRITE_ROM,
    Action::NONE_SUZY,
    Action::CPU_FETCH_OPCODE_SUZY,
    Action::CPU_FETCH_OPERAND_SUZY,
    Action::CPU_READ_SUZY,
    Action::CPU_WRITE_SUZY,
    Action::NONE_MIKEY,
    Action::CPU_FETCH_OPCODE_MIKEY,
    Action::CPU_FETCH_OPERAND_MIKEY,
    Action::CPU_READ_MIKEY,
    Action::CPU_WRITE_MIKEY,
  };

  uint64_t tick;
  size_t tableOffset{};
  switch ( request.address >> 8 )
  {
  case 0xff:
  case 0xfe:
    tick = mCurrentTick + ( ( request.address == mSequencedAccessAddress ) ? 4 : 5 );
    tableOffset = 5;
    break;
  case 0xfc:
    tick = mSuzy->requestAccess( mCurrentTick, request.address );
    tableOffset = 10;
    break;
  case 0xfd:
    tick = mMikey->requestAccess( mCurrentTick, request.address );
    tableOffset = 15;
    break;
  default:
    tick = mCurrentTick + ( ( request.address == mSequencedAccessAddress ) ? 4 : 5 );
    break;
  }

  mActionQueue.push( { requestToAction[(size_t)request.mType + tableOffset], tick } );
}


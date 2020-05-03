#include "BusMaster.hpp"
#include "CPU.hpp"
#include "Suzy.hpp"
#include "Mikey.hpp"

BusMaster::BusMaster() : mRAM{}, mBusReservationTick{}, mCurrentTick{}, mMikey{ std::make_shared<Mikey>() }, mSuzy{ std::make_shared<Suzy>() }, mActionQueue{}, mReq{}, mSequencedAccessAddress{~0u}
{
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

CPURequest * BusMaster::request( CPUReadOpcode r )
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

void BusMaster::process( uint64_t ticks )
{
  mActionQueue.push( { Action::END_FRAME, mCurrentTick + ticks } );

  for ( ;; )
  {
    auto action = mActionQueue.pop();
    mCurrentTick = action.getTick();

    switch ( action.getAction() )
    {
    case Action::CPU_READ_OPCODE:
      mReq.value = mRAM[mReq.address];
      mSequencedAccessAddress = mReq.address + 1;
      //mReq.interrupt = CPU::I_RESET;
      mReq.resume();
      break;
    case Action::CPU_READ:
      mReq.value = mRAM[mReq.address];
      mSequencedAccessAddress = mReq.address + 1;
      mReq.resume();
      break;
    case Action::CPU_WRITE:
      mRAM[mReq.address] = mReq.value;
      mSequencedAccessAddress = ~0;
      mReq.resume();
      break;
    case Action::CPU_READ_SUZY:
      mReq.value = mSuzy->read( mReq.address );
      mReq.resume();
      break;
    case Action::CPU_WRITE_SUZY:
      mSuzy->write( mReq.address, mReq.value );
      mReq.resume();
      break;
    case Action::CPU_READ_MIKEY:
      mReq.value = mMikey->read( mReq.address );
      mReq.resume();
      break;
    case Action::CPU_WRITE_MIKEY:
      mMikey->write( mReq.address, mReq.value );
      mReq.resume();
      break;
    case Action::END_FRAME:
      return;
    case Action::CPU_READ_OPCODE_SUZY:
    case Action::CPU_READ_OPCODE_MIKEY:
      //ERROR
      break;
    default:
      break;
    }
  }
}

void BusMaster::request( CPURequest const & request )
{
  static constexpr std::array<Action, 12> requestToAction = {
    Action::NONE,
    Action::CPU_READ_OPCODE,
    Action::CPU_READ,
    Action::CPU_WRITE,
    Action::NONE_SUZY,
    Action::CPU_READ_OPCODE_SUZY,
    Action::CPU_READ_SUZY,
    Action::CPU_WRITE_SUZY,
    Action::NONE_MIKEY,
    Action::CPU_READ_OPCODE_MIKEY,
    Action::CPU_READ_MIKEY,
    Action::CPU_WRITE_MIKEY,
  };

  uint64_t tick;
  size_t tableOffset{};
  switch ( request.address >> 8 )
  {
  case 0xfc:
    tick = mSuzy->requestAccess( mCurrentTick, request.address );
    tableOffset = 4;
    break;
  case 0xfd:
    tick = mMikey->requestAccess( mCurrentTick, request.address );
    tableOffset = 8;
    break;
  default:
    tick = mCurrentTick + ( ( request.address == mSequencedAccessAddress ) ? 4 : 5 );
    break;
  }

  mActionQueue.push( { requestToAction[(size_t)request.mType + tableOffset], tick } );
}

BusMaster::SequencedAction::SequencedAction( Action action, uint64_t tick ) : mData{ ( tick << TICK_PERIOD_LOG ) | (int64_t)action }
{
}

BusMaster::Action BusMaster::SequencedAction::getAction() const
{
  return (Action)( mData & ( TICK_PERIOD - 1 ) );
}

uint64_t BusMaster::SequencedAction::getTick() const
{
  return mData >> TICK_PERIOD_LOG;
}

BusMaster::SequencedAction::operator bool() const
{
  return mData != 0;
}

BusMaster::ActionQueue::ActionQueue() : mHeap{}
{
}

void BusMaster::ActionQueue::push( SequencedAction action )
{
  mHeap.push_back( action );
  std::push_heap( mHeap.begin(), mHeap.end() );
}

BusMaster::SequencedAction BusMaster::ActionQueue::pop()
{
  if ( mHeap.size() > 1 )
  {
    std::pop_heap( mHeap.begin(), mHeap.end() );
  }
  
  if ( mHeap.size() > 0 )
  {
    auto result = mHeap.back();
    mHeap.pop_back();
    return result;
  }
  else
  {
    return SequencedAction{};
  }

}

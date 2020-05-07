#include "ActionQueue.hpp"
#include <algorithm>

SequencedAction::SequencedAction( Action action, uint64_t tick ) : mData{ ( tick << TICK_PERIOD_LOG ) | (int64_t)action }
{
}

Action SequencedAction::getAction() const
{
  return (Action)( mData & ( TICK_PERIOD - 1 ) );
}

uint64_t SequencedAction::getTick() const
{
  return mData >> TICK_PERIOD_LOG;
}

SequencedAction::operator bool() const
{
  return mData != 0;
}

ActionQueue::ActionQueue() : mHeap{}
{
}

void ActionQueue::push( SequencedAction action )
{
  mHeap.push_back( action );
  std::push_heap( mHeap.begin(), mHeap.end() );
}

SequencedAction ActionQueue::pop()
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

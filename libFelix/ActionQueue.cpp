#include "pch.hpp"
#include "ActionQueue.hpp"

SequencedAction::SequencedAction() : mData{}
{
}

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

void SequencedAction::clear()
{
  mData &= ~( TICK_PERIOD - 1 );
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
  if ( !mHeap.empty() )
  {
    std::pop_heap( mHeap.begin(), mHeap.end() );

    auto result = mHeap.back();
    mHeap.pop_back();
    return result;
  }
  else
  {
    return {};
  }
}

SequencedAction ActionQueue::head() const
{
  if ( !mHeap.empty() )
  {
    return mHeap.front();
  }
  else
  {
    return {};
  }
}

void ActionQueue::erase( Action action )
{
  for ( auto& e : mHeap )
  {
    if ( e.getAction() == action )
    {
      e.clear();
    }
  }
}

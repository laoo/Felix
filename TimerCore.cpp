#include "TimerCore.hpp"

TimerCore::TimerCore( int offset, TimerCore * next ) : mNext{ next }, mOffset { offset }
{
}

SequencedAction TimerCore::setBackup( uint8_t )
{
  return {};
}

SequencedAction TimerCore::setControlA( uint8_t )
{
  return {};
}

SequencedAction TimerCore::setCount( uint8_t )
{
  return {};
}

SequencedAction TimerCore::setControlB( uint8_t )
{
  return {};
}

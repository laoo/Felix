#pragma once

#include <cstdint>
#include "ActionQueue.hpp"

class TimerCore
{
public:
  TimerCore( int offset, TimerCore * next );

  SequencedAction setBackup( uint8_t );
  SequencedAction setControlA( uint8_t );
  SequencedAction setCount( uint8_t );
  SequencedAction setControlB( uint8_t );

private:
  TimerCore * mNext;
  int mOffset;
};


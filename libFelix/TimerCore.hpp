#pragma once

#include "ActionQueue.hpp"

class TimerCore
{
public:
  TimerCore( int number, std::function<void( uint64_t, bool )> trigger );

  SequencedAction setBackup( uint64_t tick, uint8_t );
  SequencedAction setControlA( uint64_t tick, uint8_t );
  SequencedAction setCount( uint64_t tick, uint8_t );
  SequencedAction setControlB( uint64_t tick, uint8_t );

  uint8_t getBackup( uint64_t tick );
  uint8_t getControlA( uint64_t tick );
  uint8_t getCount( uint64_t tick );
  uint8_t getControlB( uint64_t tick );

  SequencedAction fireAction( uint64_t tick );
  void borrowIn( uint64_t tick );

private:
  SequencedAction computeAction();
  void updateValue( uint64_t tick );

private:
  struct CONTROLA
  {
    static constexpr uint8_t ENABLE_INT     = 0b10000000;
    static constexpr uint8_t RESET_DONE     = 0b01000000;
    static constexpr uint8_t ENABLE_RELOAD  = 0b00010000;
    static constexpr uint8_t ENABLE_COUNT   = 0b00001000;
    static constexpr uint8_t AUD_CLOCK_MASK = 0b00000111;
    static constexpr uint8_t AUD_LINKING    = 0b00000111;
  };
  struct CONTROLB
  {
    static constexpr uint8_t TIMER_DONE     = 0b00001000;
    static constexpr uint8_t LAST_CLOCK     = 0b00000100;
    static constexpr uint8_t BORROW_IN      = 0b00000010;
    static constexpr uint8_t BORROW_OUT     = 0b00000001;
  };

private:
  uint64_t mBaseTick;
  uint64_t mExpectedTick;
  uint64_t mBorrowInTick;
  uint64_t mBorrowOutTick;
  std::function<void( uint64_t, bool )> mTrigger;

  int mNumber;

  bool mEnableInt;
  bool mResetDone;
  bool mEnableReload;
  bool mEnableCount;
  bool mLinking;
  int mAudShift;
  uint8_t mValue;
  uint8_t mBackup;
  bool mTimerDone;
  bool mLastClock;
  bool mBorrowIn;
  bool mBorrowOut;

};


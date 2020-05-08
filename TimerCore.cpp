#include "TimerCore.hpp"

TimerCore::TimerCore( int number, std::function<void( uint64_t, bool )> trigger ) :
  mBaseTick{}, mExpectedTick{}, mTrigger{ std::move( trigger ) }, mNumber{ number },
  mEnableInt{}, mResetDone{}, mEnableReload{}, mEnableCount{}, mLinking{}, mAudShift{},
  mValue{},
  mBackup{},
  mTimerDone{}, mLastClock{}, mBorrowIn{}, mBorrowOut{}
{
}

SequencedAction TimerCore::setBackup( uint64_t tick, uint8_t backup )
{
  mBackup = backup;
  return {};
}

SequencedAction TimerCore::setControlA( uint64_t tick, uint8_t controlA )
{
  mEnableInt    = ( controlA & Reg::CONTROLA::ENABLE_INT ) != 0;
  mResetDone    = ( controlA & Reg::CONTROLA::RESET_DONE ) != 0;
  mEnableReload = ( controlA & Reg::CONTROLA::ENABLE_RELOAD ) != 0;
  mEnableCount  = ( controlA & Reg::CONTROLA::ENABLE_COUNT ) != 0;
  mLinking      = ( controlA & Reg::CONTROLA::AUD_LINKING ) == Reg::CONTROLA::AUD_LINKING;
  mAudShift     = controlA & Reg::CONTROLA::AUD_CLOCK_MASK;

  if ( mResetDone )
    mTimerDone = false;

  return computeAction( tick );
}

SequencedAction TimerCore::setCount( uint64_t tick, uint8_t value )
{
  mValue = value;
  return computeAction( tick );
}

SequencedAction TimerCore::setControlB( uint64_t tick, uint8_t controlB )
{
  mTimerDone  = ( controlB & Reg::CONTROLB::TIMER_DONE ) != 0;
  mLastClock  = ( controlB & Reg::CONTROLB::LAST_CLOCK ) != 0;
  mBorrowIn   = ( controlB & Reg::CONTROLB::BORROW_IN ) != 0;
  mBorrowOut  = ( controlB & Reg::CONTROLB::BORROW_OUT ) != 0;

  return computeAction( tick );
}

SequencedAction TimerCore::fireAction( uint64_t tick )
{
  if ( tick != mExpectedTick )
    return {};

  mTimerDone = true;

  mTrigger( tick, mEnableInt );

  return computeAction( tick );
}

void TimerCore::borrowIn( uint64_t tick )
{
  if ( !( mEnableCount && mLinking ) || ( mTimerDone && !mEnableReload ) )
    return;

  if ( mValue > 0 )
  {
    mLastClock = --mValue == 0;
  }
  else
  {
    if ( mLastClock )
    {
      mLastClock = false;
      mTimerDone = true;
      mTrigger( tick, mEnableInt );
    }
    if ( mEnableReload )
    {
      mValue = mBackup;
    }
  }

}

uint8_t TimerCore::value() const
{
  return mValue;
}

SequencedAction TimerCore::computeAction( uint64_t tick )
{
  if ( !mEnableCount || mLinking || ( mTimerDone && !mEnableReload ) )
    return {};

  if ( mValue == 0 && mEnableReload )
  {
    mValue = mBackup;
  }

  if ( mValue == 0 )
    return {};

  mBaseTick = tick;
  mExpectedTick = mBaseTick + ( 1ull + mValue ) * ( 1ull << mAudShift ) * 16;

  return { (Action)( ( int )Action::FIRE_TIMER0 + mNumber ), mExpectedTick };
}

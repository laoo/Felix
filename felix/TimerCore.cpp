#include "TimerCore.hpp"

TimerCore::TimerCore( int number, std::function<void( uint64_t, bool )> trigger ) :
  mBaseTick{}, mExpectedTick{}, mBorrowInTick{}, mBorrowOutTick{}, mTrigger{ std::move( trigger ) }, mNumber{ number },
  mEnableInt{}, mResetDone{}, mEnableReload{}, mEnableCount{}, mLinking{}, mAudShift{},
  mValue{},
  mBackup{},
  mTimerDone{}, mLastClock{}, mBorrowIn{}, mBorrowOut{}
{
}

SequencedAction TimerCore::setBackup( uint64_t tick, uint8_t backup )
{
  mBackup = backup;
  mValue = 0;
  return computeAction( tick );
}

SequencedAction TimerCore::setControlA( uint64_t tick, uint8_t controlA )
{
  mEnableInt    = ( controlA & CONTROLA::ENABLE_INT ) != 0;
  mResetDone    = ( controlA & CONTROLA::RESET_DONE ) != 0;
  mEnableReload = ( controlA & CONTROLA::ENABLE_RELOAD ) != 0;
  mEnableCount  = ( controlA & CONTROLA::ENABLE_COUNT ) != 0;
  mLinking      = ( controlA & CONTROLA::AUD_LINKING ) == CONTROLA::AUD_LINKING;
  mAudShift     = controlA & CONTROLA::AUD_CLOCK_MASK;

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
  mTimerDone  = ( controlB & CONTROLB::TIMER_DONE ) != 0;
  mLastClock  = ( controlB & CONTROLB::LAST_CLOCK ) != 0;
  mBorrowIn   = ( controlB & CONTROLB::BORROW_IN ) != 0;
  mBorrowOut  = ( controlB & CONTROLB::BORROW_OUT ) != 0;

  return computeAction( tick );
}

uint8_t TimerCore::getBackup( uint64_t tick )
{
  return mBackup;
}

uint8_t TimerCore::getControlA( uint64_t tick )
{
  return
    ( mEnableInt ? CONTROLA::ENABLE_INT : 0 ) |
    ( mResetDone ? CONTROLA::RESET_DONE : 0 ) |
    ( mEnableReload ? CONTROLA::ENABLE_RELOAD : 0 ) |
    ( mEnableCount ? CONTROLA::ENABLE_COUNT : 0 ) |
    mAudShift;
}

uint8_t TimerCore::getCount( uint64_t tick )
{
  if ( !mLinking )
  {
    mValue = (uint8_t)( ( tick - mBaseTick ) / ( ( 1ull << mAudShift ) * 16 ) - 1ull );
  }

  return mValue;
}

uint8_t TimerCore::getControlB( uint64_t tick )
{
  mBorrowIn = ( tick - mBorrowInTick ) < 16;
  mBorrowOut = ( tick - mBorrowOutTick ) < 16;
  mLastClock = getCount( tick ) == 0;

  return
    ( mTimerDone ? CONTROLB::TIMER_DONE : 0 ) |
    ( mLastClock ? CONTROLB::LAST_CLOCK : 0 ) |
    ( mBorrowIn ? CONTROLB::BORROW_IN : 0 ) |
    ( mBorrowOut ? CONTROLB::BORROW_OUT : 0 );
}

SequencedAction TimerCore::fireAction( uint64_t tick )
{
  if ( tick != mExpectedTick )
    return {};

  mTimerDone = true;
  mBorrowOutTick = tick;

  mTrigger( tick, mEnableInt );

  return computeAction( tick );
}

void TimerCore::borrowIn( uint64_t tick )
{
  mBorrowInTick = tick;
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

SequencedAction TimerCore::computeAction( uint64_t tick )
{
  if ( !mEnableCount || mLinking || ( mTimerDone && !mEnableReload ) )
    return {};

  if ( mValue == 0 || mEnableReload )
  {
    mValue = mBackup;
  }

  if ( mValue == 0 )
    return {};

  mBaseTick = tick;
  mExpectedTick = mBaseTick + ( 1ull + mValue ) * ( 1ull << mAudShift ) * 16;

  return { (Action)( ( int )Action::FIRE_TIMER0 + mNumber ), mExpectedTick };
}

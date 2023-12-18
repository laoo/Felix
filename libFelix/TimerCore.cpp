#include "pch.hpp"
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
  return {};
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

  //updateValue( tick & ~0x0full); //TODO: investigate why it can't be here
  mBaseTick = tick;
  return computeAction();
}

SequencedAction TimerCore::setCount( uint64_t tick, uint8_t value )
{
  mValue = value;
  mBaseTick = tick;
  return computeAction();
}

SequencedAction TimerCore::setControlB( uint64_t tick, uint8_t controlB )
{
  mTimerDone  = ( controlB & CONTROLB::TIMER_DONE ) != 0;
  mLastClock  = ( controlB & CONTROLB::LAST_CLOCK ) != 0;
  mBorrowIn   = ( controlB & CONTROLB::BORROW_IN ) != 0;
  mBorrowOut  = ( controlB & CONTROLB::BORROW_OUT ) != 0;

  return {};
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
  updateValue( tick );
  return mValue;
}

void TimerCore::updateValue( uint64_t tick )
{
  if ( !mLinking )
  {
    int64_t tmp = ( ( mExpectedTick - tick ) / ( ( 1ll << mAudShift ) * 16 ) - 1ll );
    mValue = (uint8_t)( tmp >= 0 ? tmp : 0 );
  }
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

  mBorrowOutTick = tick;
  mTrigger( tick, mEnableInt );
  mBaseTick = tick;
  if ( mEnableReload )
  {
    mValue = mBackup;
  }
  return computeAction();
}

void TimerCore::borrowIn( uint64_t tick )
{
  if ( !( mEnableCount && mLinking ) )
    return;

  mBorrowInTick = tick;

  if ( mValue > 0 )
  {
    mValue -= 1;
  }
  else
  {
    mBorrowOutTick = tick;
    mTrigger( tick, mEnableInt );
    mBaseTick = tick;
    if ( mEnableReload )
    {
      mValue = mBackup;
    }
  }
}

SequencedAction TimerCore::computeAction()
{
  if ( !mEnableCount || mLinking )
  {
    mExpectedTick = 0;
    return {};
  }

  mExpectedTick = mBaseTick + ( 1ull + mValue ) * ( 1ull << mAudShift ) * 16;

  return { (Action)( ( int )Action::FIRE_TIMER0 + mNumber ), mExpectedTick };
}

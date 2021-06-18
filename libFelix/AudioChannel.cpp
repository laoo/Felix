#include "pch.hpp"
#include "AudioChannel.hpp"
#include "TimerCore.hpp"
#include "Utility.hpp"

AudioChannel::AudioChannel( TimerCore & timer ) : mTimer{ timer }, mShiftRegisterBackup{}, mShiftRegister{}, mTapSelector{}, mEnableIntegrate{}, mVolume{}, mOutput{}
{
}

SequencedAction AudioChannel::setVolume( int8_t value )
{
  mVolume = value;
  return {};
}

SequencedAction AudioChannel::setFeedback( uint8_t value )
{
  mTapSelector = ( mTapSelector & 0b0011'1100'0000 ) | ( value & 0b0011'1111 ) | ( ( value & 0b1100'0000 ) << 4 );
  return {};
}

SequencedAction AudioChannel::setOutput( uint8_t value )
{
  mOutput = value;
  return {};
}

SequencedAction AudioChannel::setShift( uint8_t value )
{
  mShiftRegister = ( mShiftRegister & 0xff00 ) | value;
  return {};
}

SequencedAction AudioChannel::setBackup( uint64_t tick, uint8_t value )
{
  return mTimer.setBackup( tick, value );
}

SequencedAction AudioChannel::setControl( uint64_t tick, uint8_t value )
{
  mTapSelector = ( mTapSelector & 0b1111'0111'1111 ) | ( value & AUD_CONTROL::FEEDBACK_7 );
  mEnableIntegrate = ( value & AUD_CONTROL::ENABLE_INTEGRATE ) != 0;
  return mTimer.setControlA( tick, value & ~( AUD_CONTROL::FEEDBACK_7 | AUD_CONTROL::ENABLE_INTEGRATE ) );
}

SequencedAction AudioChannel::setCounter( uint64_t tick, uint8_t value )
{
  return mTimer.setCount( tick, value );
}

SequencedAction AudioChannel::setOther( uint64_t tick, uint8_t value )
{
  mShiftRegister = mShiftRegister & 0b0000'1111'1111 | ( value & 0b11110000 << 4 );
  return mTimer.setControlB( tick, value & 0b0000'1111 );
}

int8_t AudioChannel::getVolume()
{
  return mVolume;
}

uint8_t AudioChannel::getFeedback()
{
  return mTapSelector & 0b0000'0011'1111 | ( ( mTapSelector & 0b1100'0000'0000 ) >> 10 );
}

int8_t AudioChannel::getOutput()
{
  return mOutput;
}

uint8_t AudioChannel::getShift()
{
  return mShiftRegister & 0xff;
}

uint8_t AudioChannel::getBackup( uint64_t tick )
{
  return mTimer.getBackup( tick );
}

uint8_t AudioChannel::getControl( uint64_t tick )
{
  auto result = mTimer.getControlA( tick ) & ~( AUD_CONTROL::FEEDBACK_7 | AUD_CONTROL::ENABLE_INTEGRATE );

  return result |
    ( mTapSelector & 0b1000'0000 ) |
    ( mEnableIntegrate ? 0b0010'0000 : 0 );
}

uint8_t AudioChannel::getCounter( uint64_t tick )
{
  return mTimer.getCount( tick );
}

uint8_t AudioChannel::getOther( uint64_t tick )
{
  auto result = mTimer.getControlB( tick ) & 0b0000'1111;

  return result | ( ( mShiftRegister & 0b1111'0000'0000 ) >> 4 );
}

void AudioChannel::trigger()
{
  uint32_t xorGate = mTapSelector & mShiftRegister;
  uint32_t parity = popcnt( xorGate ) & 1;
  uint32_t newShift = ( mShiftRegister << 1 ) | ( parity ^ 1 );
  mShiftRegister = newShift;
  
  if ( mEnableIntegrate )
  {
    int32_t temp = mOutput + ( ( newShift & 1 ) ? mVolume : ~mVolume );
    mOutput = (int8_t)std::clamp( temp, (int32_t)std::numeric_limits<int8_t>::min(), (int32_t)std::numeric_limits<int8_t>::max() );
  }
  else
  {
    mOutput = ( newShift & 1 ) ? mVolume : ~mVolume;
  }
}

#pragma once

#include <cstdint>
#include <functional>
#include "ActionQueue.hpp"

class TimerCore;

class AudioChannel
{
public:
  AudioChannel( TimerCore & timer );

  SequencedAction setVolume( int8_t );
  SequencedAction setFeedback( uint8_t );
  SequencedAction setOutput( uint8_t );
  SequencedAction setShift( uint8_t );
  SequencedAction setBackup( uint64_t tick, uint8_t );
  SequencedAction setControl( uint64_t tick, uint8_t );
  SequencedAction setCounter( uint64_t tick, uint8_t );
  SequencedAction setOther( uint64_t tick, uint8_t );

  uint8_t getVolume();
  uint8_t getFeedback();
  uint8_t getOutput();
  uint8_t getShift();
  uint8_t getBackup( uint64_t tick );
  uint8_t getControl( uint64_t tick );
  uint8_t getCounter( uint64_t tick );
  uint8_t getOther( uint64_t tick );

  void trigger();

private:
  struct AUD_CONTROL
  {
    static constexpr uint8_t FEEDBACK_7       = 0b10000000;
    static constexpr uint8_t RESET_DONE       = 0b01000000;
    static constexpr uint8_t ENABLE_INTEGRATE = 0b00100000;
    static constexpr uint8_t ENABLE_RELOAD    = 0b00010000;
    static constexpr uint8_t ENABLE_COUNT     = 0b00001000;
    static constexpr uint8_t AUD_CLOCK_MASK   = 0b00000111;
    static constexpr uint8_t AUD_LINKING      = 0b00000111;
  };
  struct OTHER
  {
    static constexpr uint8_t LAST_CLOCK     = 0b00000100;
    static constexpr uint8_t BORROW_IN      = 0b00000010;
    static constexpr uint8_t BORROW_OUT     = 0b00000001;
  };

private:
  TimerCore & mTimer;

  uint32_t mShiftRegisterBackup;
  uint32_t mShiftRegister;
  uint32_t mTapSelector;
  bool mEnableIntegrate;
  int8_t mVolume;
  int8_t mOutput;
};


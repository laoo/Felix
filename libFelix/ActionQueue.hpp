#pragma once

static constexpr uint64_t TICK_PERIOD_LOG = 8;
static constexpr uint64_t TICK_PERIOD = 1 << TICK_PERIOD_LOG;

enum class Action
{
  NONE = 0x00,
  DISPLAY_DMA = 0x01,
  FIRE_TIMER0 = 0x02,
  FIRE_TIMER1 = 0x03,
  FIRE_TIMER2 = 0x04,
  FIRE_TIMER3 = 0x05,
  FIRE_TIMER4 = 0x06,
  FIRE_TIMER5 = 0x07,
  FIRE_TIMER6 = 0x08,
  FIRE_TIMER7 = 0x09,
  FIRE_TIMER8 = 0x0a,
  FIRE_TIMER9 = 0x0b,
  FIRE_TIMERA = 0x0c,
  FIRE_TIMERB = 0x0d,
  FIRE_TIMERC = 0x0e,
  ASSERT_IRQ = 0x10,
  ASSERT_RESET = 0x20,
  DESERT_IRQ = 0x11,
  DESERT_RESET = 0x21,
  SAMPLE_AUDIO = 0x30,
  BATCH_END = 0x40,
  ACTIONS_END_
};

static_assert( (int)Action::ACTIONS_END_ <= TICK_PERIOD );

class SequencedAction
{
public:

  SequencedAction();
  SequencedAction( Action action, uint64_t tick );

  Action getAction() const;
  uint64_t getTick() const;
  void clear();

  explicit operator bool() const;

  friend bool operator<( SequencedAction left, SequencedAction right )
  {
    return left.mData > right.mData;
  }

private:
  uint64_t mData;
};

class ActionQueue
{
public:
  ActionQueue();


  void push( SequencedAction action );
  SequencedAction pop();
  SequencedAction head() const;
  void erase( Action action );

private:

  std::vector<SequencedAction> mHeap;
};


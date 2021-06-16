#pragma once

static constexpr uint64_t TICK_PERIOD_LOG = 5;
static constexpr uint64_t TICK_PERIOD = 1 << TICK_PERIOD_LOG;

enum class Action
{
  NONE,
  DISPLAY_DMA,
  FIRE_TIMER0,
  FIRE_TIMER1,
  FIRE_TIMER2,
  FIRE_TIMER3,
  FIRE_TIMER4,
  FIRE_TIMER5,
  FIRE_TIMER6,
  FIRE_TIMER7,
  FIRE_TIMER8,
  FIRE_TIMER9,
  FIRE_TIMERA,
  FIRE_TIMERB,
  FIRE_TIMERC,
  ASSERT_IRQ,
  ASSERT_RESET,
  DESERT_IRQ,
  DESERT_RESET,
  SAMPLE_AUDIO,
  BATCH_END,
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

private:

  std::vector<SequencedAction> mHeap;
};


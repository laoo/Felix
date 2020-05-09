#pragma once

#include <cstdint>
#include <vector>

static constexpr uint64_t TICK_PERIOD_LOG = 6;
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
  CPU_FETCH_OPCODE_RAM,
  CPU_FETCH_OPERAND_RAM,
  CPU_READ_RAM,
  CPU_WRITE_RAM,
  NONE_FE,
  CPU_FETCH_OPCODE_FE,
  CPU_FETCH_OPERAND_FE,
  CPU_READ_FE,
  CPU_WRITE_FE,
  NONE_FF,
  CPU_FETCH_OPCODE_FF,
  CPU_FETCH_OPERAND_FF,
  CPU_READ_FF,
  CPU_WRITE_FF,
  NONE_SUZY,
  CPU_FETCH_OPCODE_SUZY,
  CPU_FETCH_OPERAND_SUZY,
  CPU_READ_SUZY,
  CPU_WRITE_SUZY,
  NONE_MIKEY,
  CPU_FETCH_OPCODE_MIKEY,
  CPU_FETCH_OPERAND_MIKEY,
  CPU_READ_MIKEY,
  CPU_WRITE_MIKEY,
  END_FRAME,
  ACTIONS_END_
};

static_assert( (int)Action::ACTIONS_END_ <= TICK_PERIOD );

class SequencedAction
{
public:

  SequencedAction( Action action = Action::NONE, uint64_t tick = 0 );

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

private:

  std::vector<SequencedAction> mHeap;
};


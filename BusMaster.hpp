#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>
#include "CPUExecute.hpp"
#include "CPUTrace.hpp"

class Mikey;
class Suzy;

class BusMaster
{
public:
  BusMaster();
  ~BusMaster();

  CPURequest * request( CPURead r );
  CPURequest * request( CPUFetchOpcode r );
  CPURequest * request( CPUFetchOperand r );
  CPURequest * request( CPUWrite w );
  void process( uint64_t ticks );

  TraceRequest & getTraceRequest();
private:

  void request( CPURequest const& request );


  static constexpr uint64_t TICK_PERIOD_LOG = 5;
  static constexpr uint64_t TICK_PERIOD = 1 << TICK_PERIOD_LOG;

  enum class Action
  {
    NONE,
    CPU_FETCH_OPCODE_RAM,
    CPU_FETCH_OPERAND_RAM,
    CPU_READ_RAM,
    CPU_WRITE_RAM,
    NONE_ROM,
    CPU_FETCH_OPCODE_ROM,
    CPU_FETCH_OPERAND_ROM,
    CPU_READ_ROM,
    CPU_WRITE_ROM,
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



private:

  std::array<uint8_t,65536> mRAM;
  std::array<uint8_t, 512> mROM;
  uint64_t mBusReservationTick;
  uint64_t mCurrentTick;
  std::shared_ptr<Mikey> mMikey;
  std::shared_ptr<Suzy> mSuzy;
  ActionQueue mActionQueue;
  CPURequest mReq;
  TraceRequest mDReq;
  uint32_t mSequencedAccessAddress;

};

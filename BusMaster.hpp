#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>
#include "CPUExecute.hpp"
#include "CPUTrace.hpp"
#include "ActionQueue.hpp"


class Mikey;
class Suzy;

class BusMaster
{
public:
  BusMaster( Mikey & mikey );
  ~BusMaster();

  CPURequest * request( CPURead r );
  CPURequest * request( CPUFetchOpcode r );
  CPURequest * request( CPUFetchOperand r );
  CPURequest * request( CPUWrite w );

  void process( uint64_t ticks );

  TraceRequest & getTraceRequest();
private:

  void request( CPURequest const& request );

private:
  Mikey & mMikey;

  std::array<uint8_t,65536> mRAM;
  std::array<uint8_t, 512> mROM;
  uint64_t mBusReservationTick;
  uint64_t mCurrentTick;
  std::shared_ptr<Suzy> mSuzy;
  ActionQueue mActionQueue;
  CPURequest mReq;
  TraceRequest mDReq;
  uint32_t mSequencedAccessAddress;
  uint16_t mDMAAddress;

};

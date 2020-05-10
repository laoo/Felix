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

  enum class PageType
  {
    RAM = 0,
    FE = 5,
    FF = 10,
    MIKEY = 15,
    SUZY = 20
  };

  struct MAPCTL
  {
    bool sequentialDisable;
    bool vectorSpaceDisable;
    bool kernelDisable;
    bool mikeyDisable;
    bool suzyDisable;
  };

  void request( CPURequest const& request );
  uint8_t readFF( uint16_t address );
  void writeFF( uint16_t address, uint8_t value );

private:
  Mikey & mMikey;

  std::array<uint8_t,65536> mRAM;
  std::array<uint8_t, 512> mROM;
  std::array<PageType, 256> mPageTypes;
  uint64_t mBusReservationTick;
  uint64_t mCurrentTick;
  std::shared_ptr<Suzy> mSuzy;
  ActionQueue mActionQueue;
  CPURequest mReq;
  TraceRequest mDReq;
  MAPCTL mMapCtl;
  uint32_t mSequencedAccessAddress;
  uint16_t mDMAAddress;
  uint64_t mFastCycleTick;

};

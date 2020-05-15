#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>
#include "CPUExecute.hpp"
#include "SuzyExecute.hpp"
#include "CPUTrace.hpp"
#include "ActionQueue.hpp"
#include "DisplayGenerator.hpp"


class Mikey;
class Suzy;
struct CPU;

class BusMaster
{
public:
  BusMaster();
  ~BusMaster();

  CPURequest * request( CPUFetchOpcode r );
  CPURequest * request( CPUFetchOperand r );
  CPURequest * request( CPURead r );
  CPURequest * request( CPUWrite w );
  SuzyRequest * request( SuzyFetchSCB r );
  SuzyRequest * request( SuzyFetchSprite r );
  SuzyRequest * request( SuzyReadPixel r );
  SuzyRequest * request( SuzyWritePixel w );
  void suzyStop();

  void requestDisplayDMA( uint64_t tick, uint16_t address );

  DisplayGenerator::Pixel const* process( uint64_t ticks );

  void enterMonitor();

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
  void request( SuzyRequest const& request );

  uint8_t readFF( uint16_t address );
  void writeFF( uint16_t address, uint8_t value );

private:
  std::array<uint8_t,65536> mRAM;
  std::array<uint8_t, 512> mROM;
  std::array<PageType, 256> mPageTypes;
  uint64_t mBusReservationTick;
  uint64_t mCurrentTick;
  std::shared_ptr<CPU> mCpu;
  std::shared_ptr<Mikey> mMikey;
  std::shared_ptr<Suzy> mSuzy;
  TraceRequest mDReq;
  CPURequest mCPUReq;
  SuzyRequest mSuzyReq;
  CpuExecute mCpuExecute;
  SuzyExecute mSuzyExecute;
  CpuTrace mCpuTrace;
  ActionQueue mActionQueue;
  MAPCTL mMapCtl;
  uint32_t mSequencedAccessAddress;
  uint16_t mDMAAddress;
  uint64_t mFastCycleTick;
};

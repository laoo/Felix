#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include "CPUExecute.hpp"
#include "CPUTrace.hpp"
#include "ActionQueue.hpp"
#include "DisplayGenerator.hpp"
#include "Suzy.hpp"

class Mikey;
struct CPU;
class Cartridge;
class ComLynx;
class InputFile;
class ImageBS93;

class BusMaster
{
public:
  BusMaster( std::function<void( DisplayGenerator::Pixel const* )> const& dispFun, std::function<KeyInput()> const& inputProvider );
  ~BusMaster();

  CPURequest * request( CPUFetchOpcode r );
  CPURequest * request( CPUFetchOperand r );
  CPURequest * request( CPURead r );
  CPURequest * request( CPUWrite w );
 
  CPURequest * cpuRequest();

  void requestDisplayDMA( uint64_t tick, uint16_t address );
  void assertInterrupt( int mask, std::optional<uint64_t> tick = std::nullopt );
  void desertInterrupt( int mask, std::optional<uint64_t> tick = std::nullopt );

  void process( uint64_t ticks );
  std::pair<float, float> getSample( int sps );

  void injectFile( InputFile const& file );

  void enterMonitor();

  Cartridge & getCartridge();
  ComLynx & getComLynx();

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

  void suzyRead( ISuzyProcess::RequestRead const* req );
  void suzyRead4( ISuzyProcess::RequestRead4 const* req );
  void suzyWrite( ISuzyProcess::RequestWrite const* req );
  void suzyColRMW( ISuzyProcess::RequestColRMW const* req );
  void suzyVidRMW( ISuzyProcess::RequestVidRMW const* req );
  void suzyXor( ISuzyProcess::RequestXOR const* req );

  void processCPU();
  void processSuzy();

  uint8_t readFF( uint16_t address );
  void writeFF( uint16_t address, uint8_t value );

  void loadBS93( std::shared_ptr<ImageBS93> const& image );

  void pulseReset( std::optional<uint16_t> resetAddress );
  void writeDMACTL( uint8_t value );

private:
  std::array<uint8_t,65536> mRAM;
  std::array<uint8_t, 512> mROM;
  std::array<PageType, 256> mPageTypes;
  uint64_t mBusReservationTick;
  uint64_t mCurrentTick;
  int mSamplesRemainder;
  ActionQueue mActionQueue;
  std::shared_ptr<CPU> mCpu;
  std::shared_ptr<Cartridge> mCartridge;
  std::shared_ptr<ComLynx> mComLynx;
  std::shared_ptr<Mikey> mMikey;
  std::shared_ptr<Suzy> mSuzy;
  TraceRequest mDReq;
  CPURequest mCPUReq;
  CpuExecute mCpuExecute;
  CpuTrace mCpuTrace;
  MAPCTL mMapCtl;
  uint32_t mSequencedAccessAddress;
  uint16_t mDMAAddress;
  uint64_t mFastCycleTick;
  std::shared_ptr<ISuzyProcess> mSuzyProcess;
  ISuzyProcess::Request const* mSuzyProcessRequest;
  bool mResetRequestDuringSpriteRendering;
  uint8_t mInterruptMask;
};

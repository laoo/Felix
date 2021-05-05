#pragma once

#include "ActionQueue.hpp"
#include "DisplayGenerator.hpp"
#include "Suzy.hpp"

class Mikey;
class CPU;
class Cartridge;
class ComLynx;
class InputFile;
class ImageBS93;
class ImageBIOS;
class ImageCart;

class Felix
{
public:
  Felix( std::function<void( DisplayGenerator::Pixel const* )> const& dispFun, std::function<KeyInput()> const& inputProvider );
  ~Felix();

  void requestDisplayDMA( uint64_t tick, uint16_t address );
  void runSuzy();
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
    SUZY = 4,
    MIKEY = 8,
    KERNEL = 12
  };

  struct MAPCTL
  {
    bool sequentialDisable;
    bool vectorSpaceDisable;
    bool kernelDisable;
    bool mikeyDisable;
    bool suzyDisable;
  };

  bool executeSequencedAction( SequencedAction );
  bool executeSuzyAction();
  void executeCPUAction();

  uint8_t readKernel( uint16_t address );
  void writeKernel( uint16_t address, uint8_t value );

  void pulseReset( std::optional<uint16_t> resetAddress = std::nullopt );
  void writeMAPCTL( uint8_t value );

private:
  std::array<uint8_t,65536> mRAM;
  std::array<uint8_t, 512> mROM;
  std::array<PageType, 256> mPageTypes;
  uint64_t mCurrentTick;
  int mSamplesRemainder;
  ActionQueue mActionQueue;
  std::shared_ptr<CPU> mCpu;
  std::shared_ptr<Cartridge> mCartridge;
  std::shared_ptr<ComLynx> mComLynx;
  std::shared_ptr<Mikey> mMikey;
  std::shared_ptr<Suzy> mSuzy;
  MAPCTL mMapCtl;
  uint32_t mSequencedAccessAddress;
  uint16_t mDMAAddress;
  uint64_t mFastCycleTick;
  std::shared_ptr<ISuzyProcess> mSuzyProcess;
  ISuzyProcess::Request const* mSuzyProcessRequest;
  bool mResetRequestDuringSpriteRendering;
  bool mSuzyRunning;
};

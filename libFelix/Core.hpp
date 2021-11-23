#pragma once

#include "IVideoSink.hpp"
#include "ActionQueue.hpp"
#include "DisplayGenerator.hpp"
#include "Suzy.hpp"
#include "Utility.hpp"
#include "ComLynx.hpp"
#include "ImageCart.hpp"

class Mikey;
class CPU;
class Cartridge;
class InputFile;
class ImageBS93;
class ImageBIOS;
class IEscape;
class ComLynxWire;
class TraceHelper;
class ScriptDebuggerEscapes;
class ScriptDebugger;
struct CPUState;

class Core
{
public:
  Core( std::shared_ptr<ComLynxWire> comLynxWire, std::shared_ptr<IVideoSink> videoSink, std::shared_ptr<IInputSource> inputSource, InputFile inputFile, std::optional<InputFile> kernel, std::shared_ptr<ScriptDebuggerEscapes> scriptDebuggerEscapes );
  ~Core();


  void setAudioOut( int sps, std::span<AudioSample> outputBuffer );
  int advanceAudio();

  void setLog( std::filesystem::path const & path, uint64_t startCycle );

  void enterMonitor();
  int64_t globalSamplesEmittedPerFrame() const;
  ImageCart::Rotation rotation() const;

  uint64_t tick() const;

  //Not thread safe. Used only for script escapes
  uint8_t debugReadRAM( uint16_t address ) const;
  void debugWriteRAM( uint16_t address, uint8_t value );
  uint8_t debugReadMikey( uint16_t address ) const;
  void debugWriteMikey( uint16_t address, uint8_t value );
  uint8_t debugReadSuzy( uint16_t address ) const;
  void debugWriteSuzy( uint16_t address, uint8_t value );
  CPUState& debugState();


private:

  enum class PageType
  {
    RAM = 0 * 4,
    SUZY = 1 * 4,
    MIKEY = 2 * 4,
    KERNEL = 3 * 4
  };

  struct MAPCTL
  {
    bool sequentialDisable;
    bool vectorSpaceDisable;
    bool kernelDisable;
    bool mikeyDisable;
    bool suzyDisable;
  };

  enum class SequencedActionResult
  {
    CARRY_ON,
    BAIL_OUT
  } executeSequencedAction( SequencedAction );
  bool executeSuzyAction();
  void executeCPUAction();
  void setDefaultROM();

  uint8_t fetchRAM( uint16_t address );
  uint8_t readRAM( uint16_t address );
  void writeRAM( uint16_t address, uint8_t value );
  uint8_t readMikey( uint16_t address );
  void writeMikey( uint16_t address, uint8_t value );
  uint8_t readSuzy( uint16_t address );
  void writeSuzy( uint16_t address, uint8_t value );
  uint8_t readROM( uint16_t address, bool isFetch );
  uint8_t readROM( uint16_t address );
  uint8_t fetchROM( uint16_t address );
  void writeROM( uint16_t address, uint8_t value );

  void pulseReset( std::optional<uint16_t> resetAddress = std::nullopt );
  void writeMAPCTL( uint8_t value );
  void enqueueSampling();
  void assertInterrupt( int mask, std::optional<uint64_t> tick = std::nullopt );
  void desertInterrupt( int mask, std::optional<uint64_t> tick = std::nullopt );
  void requestDisplayDMA( uint64_t tick, uint16_t address );
  void runSuzy();
  Cartridge & getCartridge();
  void newLine( int rowNr );
  std::shared_ptr<TraceHelper> getTraceHelper() const;
  uint64_t fetchRAMTiming( uint16_t address );
  uint64_t fetchROMTiming( uint16_t address );
  uint64_t readTiming( uint16_t address );
  uint64_t writeTiming( uint16_t address );

  friend class Mikey;
  friend class Suzy;
  friend class ParallelPort;

private:
  std::array<uint8_t, 65536> mRAM;
  std::array<uint8_t, 512> mROM;
  std::array<PageType, 256> mPageTypes;
  std::shared_ptr<ScriptDebugger> mScriptDebugger;
  uint64_t mCurrentTick;
  int mSamplesRemainder;
  int mSPS;
  std::span<AudioSample> mOutputSamples;
  uint32_t mSamplesEmitted;
  uint64_t mGlobalSamplesEmitted;
  uint64_t mGlobalSamplesEmittedSnapshot;
  int64_t mGlobalSamplesEmittedPerFrame;
  ActionQueue mActionQueue;
  std::shared_ptr<CPU> mCpu;
  std::shared_ptr<TraceHelper> mTraceHelper;
  std::shared_ptr<Cartridge> mCartridge;
  std::shared_ptr<ComLynx> mComLynx;
  std::shared_ptr<ComLynxWire> mComLynxWire;
  std::shared_ptr<Mikey> mMikey;
  std::shared_ptr<Suzy> mSuzy;
  MAPCTL mMapCtl;
  uint64_t mFastCycleTick;
  uint64_t mPatchMagickCodeAccumulator;
  uint32_t mLastAccessPage;
  uint16_t mDMAAddress;
  std::shared_ptr<ISuzyProcess> mSuzyProcess;
  ISuzyProcess::Request const* mSuzyProcessRequest;
  bool mResetRequestDuringSpriteRendering;
  bool mSuzyRunning;
};

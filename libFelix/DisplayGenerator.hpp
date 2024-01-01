#pragma once

#include "ParallelPort.hpp"
#include "Utility.hpp"

struct IVideoSink;

class DisplayGenerator : public RestProvider
{
public:


public:

  struct DMARequest
  {
    uint64_t tick;
    uint16_t address;

    explicit operator bool() const
    {
      return tick != 0;
    }
  };

  DisplayGenerator( std::shared_ptr<IVideoSink> videoSink );
  ~DisplayGenerator() override = default;
  void dispCtl( bool dispColor, bool dispFlip, bool dmaEnable );
  void setPBKUP( uint8_t value );

  void vblank( uint64_t tick );
  DMARequest hblank( uint64_t tick, int row );
  DMARequest pushData( uint64_t tick, uint64_t data );
  void updatePalette( uint64_t tick, uint8_t reg, uint8_t value );
  void updateDispAddr( uint64_t tick, uint16_t dispAdr );

  bool rest() const override;

private:
  bool flushDisplay( uint64_t tick );

private:
  std::array<uint64_t,10> mDMAData;
  std::shared_ptr<IVideoSink> mVideoSink;
  uint64_t mRowStartTick;
  uint32_t mDMAIteration;
  int32_t mDisplayRow;
  uint32_t mEmitedScreenBytes;
  uint16_t mDispAdr;
  bool mDispColor;
  bool mDispFlip;
  bool mDMAEnable;
  int mDMAOffset;

  static constexpr uint64_t DMA_ITERATIONS = 10;
  static constexpr uint64_t TICKS_PER_PIXEL = 12;
  static constexpr uint64_t TICKS_PER_BYTE = TICKS_PER_PIXEL * 2;
  static constexpr uint64_t ROW_TICKS = TICKS_PER_PIXEL * SCREEN_WIDTH;


};

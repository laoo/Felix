#pragma once

#include <cstdint>
#include <array>
#include <functional>

class DisplayGenerator
{
public:

  struct Pixel
  {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t x;
  };

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

  DisplayGenerator( std::function<void( DisplayGenerator::Pixel const* )> const& fun );
  void dispCtl( bool dispColor, bool dispFlip, bool dmaEnable );
  void setPBKUP( uint8_t value );

  DMARequest hblank( uint64_t tick, int row );
  DMARequest pushData( uint64_t tick, uint64_t data );
  void updatePalette( uint64_t tick, uint8_t reg, uint8_t value );
  void updateDispAddr( uint16_t dispAdr );

  void vblank( uint64_t tick );
  Pixel const* getSrface() const;

  bool rest() const;

private:
  void flushDisplay( uint64_t tick );

private:
  std::array<uint64_t,10> mDMAData;
  std::function<void( DisplayGenerator::Pixel const* )> const mDisplayFun;
  uint64_t mRowStartTick;
  uint32_t mDMAIteration;
  int32_t mDisplayRow;
  uint32_t mDisplayedPixels;
  std::array<Pixel, 160 * 102> mSurface;
  std::array<Pixel, 256> mLeftNibblePalette;
  std::array<Pixel, 256> mRightNibblePalette;
  uint16_t mDispAdr;
  bool mDispColor;
  bool mDispFlip;
  bool mDMAEnable;
  int mDMAOffset;

  static constexpr uint64_t DMA_ITERATIONS = 10;
  static constexpr uint64_t TICKS_PER_PIXEL = 12;
  static constexpr uint64_t ROW_TICKS = TICKS_PER_PIXEL * 160;


};

#pragma once

#include <cstdint>
#include <array>

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
  };

  DisplayGenerator();
  void dispCtl( bool dispColor, bool dispFlip, bool dmaEnable );

  DMARequest hblank( uint64_t tick, int row );
  DMARequest pushData( uint64_t tick, uint8_t const* data );
  void updatePalette( uint64_t tick, uint8_t reg, uint8_t value );

  void vblank( uint16_t dispAdr );
  Pixel const* getSrface() const;


private:
  std::array<uint8_t, 8> mDMAData;
  std::array<Pixel, 160 * 102> mSurface;
  uint16_t mRowAddr;
  uint16_t mDispAdr;
  bool mDispColor;
  bool mDispFlip;
  bool mDMAEnable;
 
};

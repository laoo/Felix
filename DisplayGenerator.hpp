#pragma once

#include <cstdint>
#include <array>

class DisplayGenerator
{
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

private:
  std::array<uint8_t, 8> mDMAData;
  uint16_t mRowAddr;
  uint16_t mDispAdr;
  bool mDispColor;
  bool mDispFlip;
  bool mDMAEnable;
 
};

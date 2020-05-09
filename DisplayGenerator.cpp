#include "DisplayGenerator.hpp"

DisplayGenerator::DisplayGenerator() : mDMAData{}, mRowAddr{}, mDispAdr{}, mDispColor{}, mDispFlip{}, mDMAEnable{}
{
}

void DisplayGenerator::dispCtl( bool dispColor, bool dispFlip, bool dmaEnable )
{
}

DisplayGenerator::DMARequest DisplayGenerator::hblank( uint64_t tick, int row )
{
  if ( row < 102 )
  {
    mRowAddr = ( uint16_t )( mDispAdr + ( ( 101 - row ) * 80 ) );
    return { tick, mRowAddr };
  }
  else
  {
    return {};
  }
}

DisplayGenerator::DMARequest DisplayGenerator::pushData( uint64_t tick, uint8_t const * data )
{
  mRowAddr += 8;
  return { tick + 100, mRowAddr };
}

void DisplayGenerator::updatePalette( uint64_t tick, uint8_t reg, uint8_t value )
{
}

void DisplayGenerator::vblank( uint16_t dispAdr )
{
  mDispAdr = dispAdr;
}

DisplayGenerator::Pixel const * DisplayGenerator::getSrface() const
{
  return mSurface.data();
}

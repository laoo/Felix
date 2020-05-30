#include "DisplayGenerator.hpp"

DisplayGenerator::DisplayGenerator( std::function<void( DisplayGenerator::Pixel const* )> const& fun ) : mDMAData{}, mDisplayFun{ fun }, mRowStartTick{}, mDMAIteration{}, mDisplayRow{}, mDisplayedPixels{},
mDispAdr{}, mDispColor{}, mDispFlip{}, mDMAEnable{}
{
  for ( uint32_t i = 0; i < 256; ++i )
  {
    mLeftNibblePalette[i] = Pixel{ 0, 0, 0, 255 };
    mRightNibblePalette[i] = Pixel{ 0, 0, 0, 255 };
  }
}

void DisplayGenerator::dispCtl( bool dispColor, bool dispFlip, bool dmaEnable )
{
  mDispColor = dispColor;
  mDispFlip = dispFlip;
  mDMAEnable = dmaEnable;
}

DisplayGenerator::DMARequest DisplayGenerator::hblank( uint64_t tick, int row )
{
  flushDisplay( tick );
  mDisplayRow = 101 - row;
  if ( mDisplayRow >= 0 && mDMAEnable )
  {
    mRowStartTick = tick;
    mDMAIteration = 0;
    mDisplayedPixels = 0;
    return { mRowStartTick + mDMAIteration * ( ROW_TICKS / DMA_ITERATIONS ), mDispAdr };
  }
  else
  {
    mRowStartTick = 0;
    return {};
  }
}

DisplayGenerator::DMARequest DisplayGenerator::pushData( uint64_t tick, uint64_t data )
{
  mDMAData[mDMAIteration] = data;
  flushDisplay( tick );
  mDispAdr += ( 80 / DMA_ITERATIONS );
  if ( ++mDMAIteration < 10 )
  {
    return { mRowStartTick + mDMAIteration * ( ROW_TICKS / DMA_ITERATIONS ), mDispAdr };
  }
  else
  {
    return {};
  }
}

void DisplayGenerator::updatePalette( uint64_t tick, uint8_t reg, uint8_t value )
{
  flushDisplay( tick );

  if ( reg < 16 )
  {
    uint32_t regLo = reg;
    uint32_t regHi = reg << 4;

    //green
    uint8_t g = ( value << 4 ) | ( value & 0x0f );

    for ( uint32_t i = 0; i < 256; ++i )
    {
      if ( ( i & 0xf0 ) == regHi )
      {
        mLeftNibblePalette[i].g = g;
      }
      if ( ( i & 0x0f ) == regLo )
      {
        mRightNibblePalette[i].g = g;
      }
    }
  }
  else
  {
    uint32_t regLo = reg & 0x0f;
    uint32_t regHi = regLo << 4;

    //blue
    uint8_t b = ( value >> 4 ) | ( value & 0xf0 );
    //red
    uint8_t r = ( value << 4 ) | ( value & 0x0f );

    for ( uint32_t i = 0; i < 256; ++i )
    {
      if ( ( i & 0xf0 ) == regHi )
      {
        mLeftNibblePalette[i].b = b;
        mLeftNibblePalette[i].r = r;
      }
      if ( ( i & 0x0f ) == regLo )
      {
        mRightNibblePalette[i].b = b;
        mRightNibblePalette[i].r = r;
      }
    }
  }
}

void DisplayGenerator::updateDispAddr( uint16_t dispAdr )
{
  mDispAdr = dispAdr;
}

void DisplayGenerator::vblank( uint64_t tick )
{
  flushDisplay( tick );
  if ( mDisplayFun )
    mDisplayFun( mSurface.data() );
}

void DisplayGenerator::flushDisplay( uint64_t tick )
{
  if ( mRowStartTick == 0 )
    return;

  uint32_t limit = (std::min)( 160u, ( uint32_t )( tick - mRowStartTick ) / ( uint32_t )TICKS_PER_PIXEL );

  uint8_t const* lineData = reinterpret_cast<uint8_t const*>( mDMAData.data() );

  while ( mDisplayedPixels < limit )
  {
    uint8_t b = lineData[mDisplayedPixels >> 1];
    mSurface[( uint32_t )( mDisplayRow * 160 + mDisplayedPixels )] = mDisplayedPixels & 1 ? mRightNibblePalette[b] : mLeftNibblePalette[b];
    mDisplayedPixels += 1;
  }
}

DisplayGenerator::Pixel const * DisplayGenerator::getSrface() const
{
  return mSurface.data();
}

bool DisplayGenerator::rest() const
{
  return mDisplayRow < 103 && mDisplayRow > 99;
}


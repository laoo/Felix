#include "DisplayGenerator.hpp"
#include "IVideoSink.hpp"
#include "Log.hpp"

DisplayGenerator::DisplayGenerator( std::shared_ptr<IVideoSink> videoSink ) :
  mDMAData{}, mVideoSink{ std::move( videoSink ) }, mRowStartTick{ std::numeric_limits<uint64_t>::max() }, mDMAIteration{}, mDisplayRow{}, mEmittedRowDoublets{},
  mDispAdr{}, mDispColor{}, mDispFlip{}, mDMAEnable{}, mDMAOffset{ -1 }, mRowPtr{}
{
  assert( mVideoSink );
  std::ranges::fill( mPalette, 0 );
  std::ranges::fill( mDoublets, Doublet{} );
}

void DisplayGenerator::dispCtl( bool dispColor, bool dispFlip, bool dmaEnable )
{
  mDMAEnable = dmaEnable;
  //TODO: handle dispColor and dispFlip
  mDispColor = dispColor;
  mDispFlip = dispFlip;
}

void DisplayGenerator::setPBKUP( uint8_t value )
{
  int h = (int)std::round( ( (int)value + 1.0 ) / 4.0 * 15.0 + 0.49 );
  mDMAOffset = h * 16 - ROW_TICKS;
}

void DisplayGenerator::vblank( uint64_t tick )
{
  if ( mDMAEnable )
  {
    flushDisplay( tick );
  }
  mVideoSink->newFrame();
  mRowStartTick = std::numeric_limits<uint64_t>::max();
}

DisplayGenerator::DMARequest DisplayGenerator::hblank( uint64_t tick, int row )
{
  if ( mDMAEnable )
  {
    flushDisplay( tick );
  }
  mDMAIteration = 0;
  mEmittedRowDoublets = 0;
  mDisplayRow = 101 - row;
  if ( mDisplayRow >= 0 && mDMAOffset >= 0 )
  {
    mRowPtr = mVideoSink->getRow( mDisplayRow );
    mRowStartTick = tick + mDMAOffset;
    if ( mDMAEnable )
    {
      return { mRowStartTick, mDispAdr };
    }
  }

  return {};
}

DisplayGenerator::DMARequest DisplayGenerator::pushData( uint64_t tick, uint64_t data )
{
  if ( mDMAIteration < DMA_ITERATIONS )
  {
    mDMAData[mDMAIteration] = data;
    flushDisplay( tick );
    mDispAdr += ( DMA_FETCH_SIZE );
    if ( ++mDMAIteration < DMA_ITERATIONS )
    {
      return { mRowStartTick + mDMAIteration * ( ROW_TICKS / DMA_ITERATIONS ), mDispAdr };
    }
  }
  return {};
}

void DisplayGenerator::updateDispAddr( uint64_t tick, uint16_t dispAdr )
{
  mDispAdr = dispAdr;
}

void DisplayGenerator::setColorReg( uint64_t tick, uint8_t reg, uint8_t value )
{
  flushDisplay( tick );

  if ( reg < 16 )
  {
    mPalette[reg] = value & 0x0f;
    uint32_t regLo = reg;
    uint32_t regHi = reg << 4;

    //green
    uint8_t g = ( value << 4 ) | ( value & 0x0f );

    for ( uint32_t i = regHi; i < regHi + 16; ++i )
    {
      mDoublets[i].left.g = g;
    }
    for ( uint32_t i = regLo; i < 256; i += 16 )
    {
      mDoublets[i].right.g = g;
    }
  }
  else
  {
    mPalette[reg] = value;
    uint32_t regLo = reg & 0x0f;
    uint32_t regHi = regLo << 4;

    //blue
    uint8_t b = ( value >> 4 ) | ( value & 0xf0 );
    //red
    uint8_t r = ( value << 4 ) | ( value & 0x0f );

    for ( uint32_t i = regHi; i < regHi + 16; ++i )
    {
      mDoublets[i].left.b = b;
      mDoublets[i].left.r = r;
    }
    for ( uint32_t i = regLo; i < 256; i += 16 )
    {
      mDoublets[i].right.b = b;
      mDoublets[i].right.r = r;
    }
  }
}

uint8_t DisplayGenerator::getColorReg( uint8_t reg ) const
{
  return mPalette[reg];
}

std::span<uint8_t const, 32> DisplayGenerator::debugPalette() const
{
  return std::span<uint8_t const, 32>( mPalette.data(), mPalette.size() );
}

void DisplayGenerator::flushDisplay( uint64_t tick )
{
  if ( tick > mRowStartTick )
  {
    for ( uint32_t limit = ( std::min )( ROW_BYTES, ( uint32_t )( tick - mRowStartTick ) / ( ( uint32_t )TICKS_PER_BYTE ) ); mEmittedRowDoublets < limit; ++mEmittedRowDoublets )
    {
      //NOTICE - pixels are processed in byte pairs, so in this implementation it is not possible to alter color register between nibbles of a screen byte
      *mRowPtr++ = mDoublets[std::bit_cast< uint8_t const* >( mDMAData.data() )[mEmittedRowDoublets]];
    }
  }
}

bool DisplayGenerator::rest() const
{
  return mDisplayRow < 103 && mDisplayRow > 99;
}


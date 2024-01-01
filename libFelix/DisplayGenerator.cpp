#include "DisplayGenerator.hpp"
#include "IVideoSink.hpp"
#include "Log.hpp"

DisplayGenerator::DisplayGenerator( std::shared_ptr<IVideoSink> videoSink ) : mDMAData{}, mVideoSink{ std::move( videoSink ) }, mRowStartTick{ std::numeric_limits<uint64_t>::max() }, mDMAIteration{}, mDisplayRow{}, mEmitedScreenBytes{},
  mDispAdr{}, mDispColor{}, mDispFlip{}, mDMAEnable{}, mDMAOffset{ -1 }
{
  assert( mVideoSink );
}

void DisplayGenerator::dispCtl( bool dispColor, bool dispFlip, bool dmaEnable )
{
  mDispColor = dispColor;
  mDispFlip = dispFlip;
  mDMAEnable = dmaEnable;
}

void DisplayGenerator::setPBKUP( uint8_t value )
{
  int h = (int)std::round( ( (int)value + 1.0 ) / 4.0 * 15.0 + 0.49 );
  mDMAOffset = h * 16 - ROW_TICKS;
}


void DisplayGenerator::vblank( uint64_t tick )
{
  flushDisplay( tick );
  mEmitedScreenBytes = 0;
  mDMAIteration = 0;
  mVideoSink->newFrame();
  mRowStartTick = std::numeric_limits<uint64_t>::max();
}

DisplayGenerator::DMARequest DisplayGenerator::hblank( uint64_t tick, int row )
{
  flushDisplay( tick );
  mEmitedScreenBytes = 0;
  mDMAIteration = 0;
  mDisplayRow = 101 - row;
  mVideoSink->newRow( row );
  if ( mDisplayRow >= 0 && mDMAOffset >= 0 )
  {
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
  if ( mDMAIteration < 10 )
  {
    mDMAData[mDMAIteration] = data;
    flushDisplay( tick );
    mDispAdr += ( 80 / DMA_ITERATIONS );
    if ( ++mDMAIteration < 10 )
    {
      return { mRowStartTick + mDMAIteration * ( ROW_TICKS / DMA_ITERATIONS ), mDispAdr };
    }
  }
  return {};
}

void DisplayGenerator::updatePalette( uint64_t tick, uint8_t reg, uint8_t value )
{
  flushDisplay( tick );
  mVideoSink->updateColorReg( reg, value );
}

void DisplayGenerator::updateDispAddr( uint64_t tick, uint16_t dispAdr )
{
  mDispAdr = dispAdr;
}

void DisplayGenerator::flushDisplay( uint64_t tick )
{
  if ( !mDMAEnable )
    return;

  if ( tick <= mRowStartTick )
    return;

  uint32_t limit = (std::min)( 80u, ( uint32_t )( tick - mRowStartTick ) / ( ( uint32_t )TICKS_PER_BYTE ) );

  if ( size_t bytesToEmit = limit - mEmitedScreenBytes )
  {
    //NOTICE - pixels are processed in byte pairs, so in this implementation it is not possible to alter color register between nibbles of a screen byte
    mVideoSink->emitScreenData( std::span<uint8_t const>( std::bit_cast< uint8_t const* >( mDMAData.data() ) + mEmitedScreenBytes, bytesToEmit ) );
  }
  mEmitedScreenBytes = limit;
}

bool DisplayGenerator::rest() const
{
  return mDisplayRow < 103 && mDisplayRow > 99;
}


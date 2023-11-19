#include "pch.hpp"
#include "VideoSink.hpp"
#include "ScreenRenderingBuffer.hpp"

void VideoSink::newFrame( uint64_t tick, uint8_t hbackup )
{
  mFrameTicks = tick - mBeginTick;
  mBeginTick = tick;
  if ( mActiveFrame )
  {
    std::scoped_lock<std::mutex> lock( mQueueMutex );
    if ( mFinishedFrames.size() > 1 )
    {
      mFinishedFrames.pop();
    }
    mFinishedFrames.push( std::move( mActiveFrame ) );
    mActiveFrame.reset();
  }
  mActiveFrame = std::make_shared<ScreenRenderingBuffer>();
}

void VideoSink::newRow(uint64_t tick, int row)
{
    if (mActiveFrame) {
    mActiveFrame->newRow(row);
}
}

void VideoSink::updatePalette( uint16_t reg, uint8_t value )
{
  reg &= 0xff;

  if ( reg < 16 )
  {
    uint32_t regLo = reg;
    uint32_t regHi = reg << 4;

    //green
    uint8_t g = ( value << 4 ) | ( value & 0x0f );

    for ( uint32_t i = regHi; i < regHi + 16; ++i )
    {
      mPalette[i].left.g = g;
    }
    for ( uint32_t i = regLo; i < 256; i += 16 )
    {
      mPalette[i].right.g = g;
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

    for ( uint32_t i = regHi; i < regHi + 16; ++i )
    {
      mPalette[i].left.b = b;
      mPalette[i].left.r = r;
    }
    for ( uint32_t i = regLo; i < 256; i += 16 )
    {
      mPalette[i].right.b = b;
      mPalette[i].right.r = r;
    }
  }
}

std::shared_ptr<ScreenRenderingBuffer> VideoSink::pullNextFrame()
{
  std::shared_ptr<ScreenRenderingBuffer> result{};

  std::scoped_lock<std::mutex> lock( mQueueMutex );
  if ( !mFinishedFrames.empty() )
  {
    result = mFinishedFrames.front();
    mFinishedFrames.pop();
  }

  return result;
}

void VideoSink::emitScreenData( std::span<uint8_t const> data )
{
  if ( mActiveFrame )
    mActiveFrame->pushScreenBytes( data );
}

void VideoSink::updateColorReg( uint8_t reg, uint8_t value )
{
  if ( mActiveFrame )
  {
    mActiveFrame->pushColorChage( reg, value );
  }
  else
  {
    updatePalette( reg, value );
  }
}

VideoSink::VideoSink() : mActiveFrame{}, mFinishedFrames{}, mQueueMutex{}, mBeginTick{}, mLastTick{}, mFrameTicks{ ~0ull }
{
  for ( uint32_t i = 0; i < 256; ++i )
  {
    mPalette[i] = DPixel{ Pixel{ 0, 0, 0, 255 }, Pixel{ 0, 0, 0, 255 } };
  }
}

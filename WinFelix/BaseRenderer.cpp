#include "pch.hpp"
#include "BaseRenderer.hpp"
#include "ScreenRenderingBuffer.hpp"
#include "imgui.h"

void BaseRenderer::Instance::newFrame( uint64_t tick, uint8_t hbackup )
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

void BaseRenderer::Instance::newRow( uint64_t tick, int row )
{
  mActiveFrame->newRow( row );
}

void BaseRenderer::Instance::updatePalette( uint16_t reg, uint8_t value )
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

std::shared_ptr<ScreenRenderingBuffer> BaseRenderer::Instance::pullNextFrame()
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

void BaseRenderer::Instance::emitScreenData( std::span<uint8_t const> data )
{
  if ( mActiveFrame )
    mActiveFrame->pushScreenBytes( data );
}

void BaseRenderer::Instance::updateColorReg( uint8_t reg, uint8_t value )
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

BaseRenderer::Instance::Instance() : mActiveFrame{}, mFinishedFrames{}, mQueueMutex{}, mBeginTick{}, mLastTick{}, mFrameTicks{ ~0ull }
{
  for ( uint32_t i = 0; i < 256; ++i )
  {
    mPalette[i] = DPixel{ Pixel{ 0, 0, 0, 255 }, Pixel{ 0, 0, 0, 255 } };
  }
}

BaseRenderer::BaseRenderer( HWND hWnd ) : mHWnd{ hWnd }, mInstance{ std::make_shared<Instance>() }, mScreenGeometry{}, mRotation{}
{
}

ImTextureID BaseRenderer::renderBoard( int id, int width, int height, std::span<uint8_t const> data )
{
  return ImTextureID{};
}

void* BaseRenderer::mainRenderingTexture( int width, int height )
{
  return ImTextureID{};
}

void* BaseRenderer::screenViewRenderingTexture( int id, ScreenViewType type, std::span<uint8_t const> data, std::span<uint8_t const> palette, int width, int height )
{
  return ImTextureID{};
}

bool BaseRenderer::canRenderBoards() const
{
  return false;
}

std::shared_ptr<IVideoSink> BaseRenderer::getVideoSink() const
{
  return mInstance;
}

int BaseRenderer::sizing( RECT& rect )
{
  RECT wRect, cRect;
  GetWindowRect( mHWnd, &wRect );
  GetClientRect( mHWnd, &cRect );

  int lastW = wRect.right - wRect.left;
  int lastH = wRect.bottom - wRect.top;
  int newW = rect.right - rect.left;
  int newH = rect.bottom - rect.top;
  int dW = newW - lastW;
  int dH = newH - lastH;

  int cW = cRect.right - cRect.left + dW;
  int cH = cRect.bottom - cRect.top + dH;

  if ( cW < mScreenGeometry.minWindowWidth() )
  {
    rect.left = wRect.left;
    rect.right = wRect.right;
  }
  if ( cH < mScreenGeometry.minWindowHeight() )
  {
    rect.top = wRect.top;
    rect.bottom = wRect.bottom;
  }

  return 1;
}

void BaseRenderer::setRotation( ImageProperties::Rotation rotation )
{
  mRotation = rotation;
}

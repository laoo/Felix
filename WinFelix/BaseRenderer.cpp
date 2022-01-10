#include "pch.hpp"
#include "BaseRenderer.hpp"
#include "ScreenRenderingBuffer.hpp"
#include "imgui.h"
#include "DX9Renderer.hpp"
#include "DX11Renderer.hpp"
#include "VideoSink.hpp"


std::shared_ptr<BaseRenderer> BaseRenderer::createRenderer( HWND hWnd, std::filesystem::path const& iniPath )
{
  try
  {
    return std::make_shared<DX11Renderer>( hWnd, iniPath );
  }
  catch ( ... )
  {
    return std::make_shared<DX9Renderer>( hWnd, iniPath );
  }
}


BaseRenderer::BaseRenderer( HWND hWnd ) : mHWnd{ hWnd }, mVideoSink{ std::make_shared<VideoSink>() }, mScreenGeometry{}, mRotation{}, mLastRenderTimePoint{}
{
  LARGE_INTEGER l;
  QueryPerformanceCounter( &l );
  mLastRenderTimePoint = l.QuadPart;
}

int64_t BaseRenderer::render( UI& ui )
{
  LARGE_INTEGER l;
  QueryPerformanceCounter( &l );

  internalRender( ui );
  present();

  auto result = l.QuadPart - mLastRenderTimePoint;
  mLastRenderTimePoint = l.QuadPart;
  return result;
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
  return mVideoSink;
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



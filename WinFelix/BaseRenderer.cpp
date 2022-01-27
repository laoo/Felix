#include "pch.hpp"
#include "BaseRenderer.hpp"
#include "ScreenRenderingBuffer.hpp"
#include "imgui.h"
#include "DX9Renderer.hpp"
#include "DX11Renderer.hpp"
#include "WinImgui.hpp"
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


BaseRenderer::BaseRenderer( HWND hWnd ) : mHWnd{ hWnd }, mImgui{}, mVideoSink{ std::make_shared<VideoSink>() }, mScreenGeometry{}, mRotation{}, mLastRenderTimePoint{}
{
  LARGE_INTEGER l;
  QueryPerformanceCounter( &l );
  mLastRenderTimePoint = l.QuadPart;
}

BaseRenderer::~BaseRenderer()
{
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

void BaseRenderer::setEncoder( std::shared_ptr<IEncoder> encoder )
{
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
  updateRotation();
}


int BaseRenderer::win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch ( msg )
  {
  case WM_SIZING:
    return sizing( *(RECT*)lParam );
  default:
    if ( mImgui )
      return mImgui->win32_WndProcHandler( hWnd, msg, wParam, lParam );
  }

  return 0;
}

#include "pch.hpp"
#include "WinRenderer.hpp"
#include "Manager.hpp"
#include "imgui.h"
#include "WinImgui11.hpp"
#include "WinImgui9.hpp"
#include "Log.hpp"
#include "IEncoder.hpp"
#include "ScreenRenderingBuffer.hpp"
#include "DX9Renderer.hpp"
#include "DX11Renderer.hpp"



WinRenderer::WinRenderer() : mRenderer{}
{
}

WinRenderer::~WinRenderer()
{
}

void WinRenderer::setEncoder( std::shared_ptr<IEncoder> encoder )
{
  if ( mRenderer )
    mRenderer->setEncoder( std::move( encoder ) );
}

void WinRenderer::initialize( HWND hWnd, std::filesystem::path const& iniPath )
{
  try
  {
    mRenderer = std::make_shared<DX11Renderer>( hWnd, iniPath );
  }
  catch( ... )
  {
    mRenderer = std::make_shared<DX9Renderer>( hWnd, iniPath );
  }
}

std::shared_ptr<IVideoSink> WinRenderer::getVideoSink() const
{
  assert( mRenderer );
  return mRenderer->getVideoSink();
}

int64_t WinRenderer::render( Manager& config )
{
  return mRenderer->render( config );
}

bool WinRenderer::canRenderBoards() const
{
  return mRenderer->canRenderBoards();
}

void* WinRenderer::renderBoard( int id, int width, int height, std::span<uint8_t const> data )
{
  return mRenderer->renderBoard( id, width, height, data );
}

void* WinRenderer::mainRenderingTexture( int width, int height )
{
  return mRenderer->mainRenderingTexture( width, height );
}

void* WinRenderer::screenViewRenderingTexture( int id, ScreenViewType type, std::span<uint8_t const> data, std::span<uint8_t const> palette, int width, int height )
{
  return mRenderer->screenViewRenderingTexture( id, type, data, palette, width, height );
}

int WinRenderer::win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  return mRenderer->win32_WndProcHandler( hWnd, msg, wParam, lParam );
}

void WinRenderer::setRotation( ImageProperties::Rotation rotation )
{
  assert( mRenderer );
  mRenderer->setRotation( rotation );
}


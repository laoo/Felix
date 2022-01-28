#include "pch.hpp"
#include "SystemDriver.hpp"
#include "DX9Renderer.hpp"
#include "DX11Renderer.hpp"

SystemDriver::SystemDriver( HWND hWnd, std::filesystem::path const& iniPath ) : mhWnd{ hWnd }
{
  try
  {
    std::tie( mBaseRenderer, mExtendedRenderer ) = DX11Renderer::create( hWnd, iniPath );
  }
  catch ( ... )
  {
    std::tie( mBaseRenderer, mExtendedRenderer ) = DX9Renderer::create( hWnd, iniPath );
  }
}

std::shared_ptr<IBaseRenderer> SystemDriver::baseRenderer() const
{
  return mBaseRenderer;
}

std::shared_ptr<IExtendedRenderer> SystemDriver::extendedRenderer() const
{
  return mExtendedRenderer;
}

int SystemDriver::wndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  return mBaseRenderer->wndProcHandler( hWnd, msg, wParam, lParam );
}

void SystemDriver::quit()
{
  PostMessage( mhWnd, WM_CLOSE, 0, 0 );
}

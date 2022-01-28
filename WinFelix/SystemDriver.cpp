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
  switch ( msg )
  {
  case WM_DROPFILES:
    handleFileDrop( (HDROP)wParam );
    SetForegroundWindow( hWnd );
    ShowWindow( hWnd, SW_RESTORE );
    return 0;
  default:
    assert( mBaseRenderer );
    return mBaseRenderer->wndProcHandler( hWnd, msg, wParam, lParam );
  }
}

void SystemDriver::quit()
{
  PostMessage( mhWnd, WM_CLOSE, 0, 0 );
}

void SystemDriver::registerDropFiles( std::function<void( std::filesystem::path )> handler )
{
  mDropFilesHandler = std::move( handler );
}

void SystemDriver::handleFileDrop( HDROP hDrop )
{
#ifdef _WIN64
  auto h = GlobalAlloc( GMEM_MOVEABLE, 0 );
  uintptr_t hptr = reinterpret_cast<uintptr_t>( h );
  GlobalFree( h );
  uintptr_t hdropptr = reinterpret_cast<uintptr_t>( hDrop );
  hDrop = reinterpret_cast<HDROP>( hptr & 0xffffffff00000000 | hdropptr & 0xffffffff );
#endif

  uint32_t cnt = DragQueryFile( hDrop, ~0, nullptr, 0 );

  std::wstring arg;

  if ( cnt > 0 )
  {
    uint32_t size = DragQueryFile( hDrop, 0, nullptr, 0 );
    arg.resize( size + 1, L'\0' );
    DragQueryFile( hDrop, 0, arg.data(), size + 1 );
  }

  DragFinish( hDrop );
  if ( mDropFilesHandler )
    mDropFilesHandler( std::move( arg ) );
}

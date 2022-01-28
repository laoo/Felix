#include "pch.hpp"
#include "SystemDriver.hpp"
#include "DX9Renderer.hpp"
#include "DX11Renderer.hpp"
#include "ConfigProvider.hpp"
#include "SysConfig.hpp"
#include "UserInput.hpp"

SystemDriver::SystemDriver( HWND hWnd, std::filesystem::path const& iniPath ) : mhWnd{ hWnd }, mIntputSource{ std::make_shared<UserInput>() }
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
  RECT r;
  switch ( msg )
  {
  case WM_CLOSE:
    if ( GetWindowRect( hWnd, &r ) )
    {
      auto sysConfig = gConfigProvider.sysConfig();
      sysConfig->mainWindow.x = r.left;
      sysConfig->mainWindow.y = r.top;
      sysConfig->mainWindow.width = r.right - r.left;
      sysConfig->mainWindow.height = r.bottom - r.top;
    }
    DestroyWindow( hWnd );
    return 0;
  case WM_DESTROY:
    PostQuitMessage( 0 );
    return 0;
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
    if ( wParam < 256 )
    {
      mIntputSource->keyDown( (int)wParam );
    }
    return mBaseRenderer->wndProcHandler( hWnd, msg, wParam, lParam );
  case WM_KEYUP:
  case WM_SYSKEYUP:
    if ( wParam < 256 )
    {
      mIntputSource->keyUp( (int)wParam );
    }
    return mBaseRenderer->wndProcHandler( hWnd, msg, wParam, lParam );
  case WM_DROPFILES:
    handleFileDrop( (HDROP)wParam );
    SetForegroundWindow( hWnd );
    ShowWindow( hWnd, SW_RESTORE );
    return 0;
  case WM_KILLFOCUS:
    mIntputSource->lostFocus();
    return mBaseRenderer->wndProcHandler( hWnd, msg, wParam, lParam );
  case WM_DEVICECHANGE:
    if ( (UINT)wParam == DBT_DEVNODES_CHANGED )
      mIntputSource->recheckGamepad();
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

void SystemDriver::update()
{
  mIntputSource->updateGamepad();
}

std::shared_ptr<IUserInput> SystemDriver::userInput() const
{
  return mIntputSource;
}

void SystemDriver::updateRotation( ImageProperties::Rotation rotation )
{
  mIntputSource->setRotation( rotation );
  mBaseRenderer->setRotation( rotation );
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

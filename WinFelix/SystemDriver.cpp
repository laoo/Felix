#include "pch.hpp"
#include "SystemDriver.hpp"
#include "DX9Renderer.hpp"
#include "DX11Renderer.hpp"
#include "ConfigProvider.hpp"
#include "SysConfig.hpp"
#include "UserInput.hpp"
#include "version.hpp"
#include "Manager.hpp"

static constexpr wchar_t gClassName[] = L"FelixEmulatorWindowClass";

LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch ( msg )
  {
  case WM_CREATE:
  {
    SystemDriver* systemDriver = reinterpret_cast<SystemDriver*>( reinterpret_cast<LPCREATESTRUCT>( lParam )->lpCreateParams );
    assert( systemDriver );
    try
    {
      systemDriver->initialize( hwnd );
    }
    catch ( std::exception const& ex )
    {
      MessageBoxA( nullptr, ex.what(), "Renderinit Error", MB_OK | MB_ICONERROR );
      PostQuitMessage( 0 );
    }
    SetWindowLongPtr( hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( systemDriver ) );
    break;
  }
  default:
    if ( SystemDriver* systemDriver = reinterpret_cast<SystemDriver*>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) ) )
    {
      if ( systemDriver->wndProcHandler( hwnd, msg, wParam, lParam ) )
        return 1;
    }
    return DefWindowProc( hwnd, msg, wParam, lParam );
  }

  return 0;
}


bool runOtherInstanceIfPresent( std::wstring const& arg )
{
  if ( auto hwnd = FindWindowW( gClassName, nullptr ) )
  {
    std::vector<wchar_t> buffer;
    if ( !arg.empty() )
    {
      std::copy( arg.cbegin(), arg.cend(), std::back_inserter( buffer ) );
    }
    buffer.push_back( 0 );
    buffer.push_back( 0 );

    HGLOBAL hMem = GlobalAlloc( GHND, sizeof( DROPFILES ) + buffer.size() * sizeof( wchar_t ) );

    if ( auto dropFiles = (DROPFILES*)GlobalLock( hMem ) )
    {
      dropFiles->pFiles = sizeof( DROPFILES );
      dropFiles->pt = POINT{};
      dropFiles->fNC = false;
      dropFiles->fWide = true;
      memcpy( &dropFiles[1], &buffer[0], buffer.size() * sizeof( wchar_t ) );
      PostMessage( hwnd, WM_DROPFILES, (WPARAM)hMem, 0 );
      GlobalUnlock( hMem );
    }

    GlobalFree( hMem );
    return true;
  }

  return false;
}

std::shared_ptr<ISystemDriver> createSystemDriver( Manager& manager, std::wstring const& arg, int nCmdShow )
{
  auto sysConfig = gConfigProvider.sysConfig();

  if ( sysConfig->singleInstance )
  {
    if ( runOtherInstanceIfPresent( arg ) )
    {
      return {};
    }
  }

  WNDCLASSEX wc{};

  wc.cbSize = sizeof( WNDCLASSEX );
  wc.style = 0;
  wc.lpfnWndProc = WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = (HINSTANCE)&__ImageBase;
  wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
  wc.hCursor = LoadCursor( NULL, IDC_ARROW );
  wc.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
  wc.lpszMenuName = NULL;
  wc.lpszClassName = gClassName;
  wc.hIconSm = LoadIcon( NULL, IDI_APPLICATION );

  if ( !RegisterClassEx( &wc ) )
  {
    return {};
  }

#ifdef FELIX86
  std::wstring name = L"Felix32 " + std::wstring{ version_string };
#else
std::wstring name = L"Felix " + std::wstring{ version_string };
#endif

  auto pSystemDriver = std::make_shared<SystemDriver>();

  HWND hwnd = CreateWindowEx( WS_EX_CLIENTEDGE, gClassName, name.c_str(), WS_OVERLAPPEDWINDOW, sysConfig->mainWindow.x, sysConfig->mainWindow.y, sysConfig->mainWindow.width, sysConfig->mainWindow.height, nullptr, nullptr, wc.hInstance, pSystemDriver.get() );

  if ( !hwnd )
  {
    return {};
  }

  ShowWindow( hwnd, nCmdShow );
  UpdateWindow( hwnd );
  DragAcceptFiles( hwnd, TRUE );

  manager.initialize( pSystemDriver );

  if ( pSystemDriver->mDropFilesHandler )
  {
    pSystemDriver->mDropFilesHandler( std::move( arg ) );
  }

  return pSystemDriver;
}

SystemDriver::SystemDriver() : mhWnd{}, mIntputSource{}
{
}

void SystemDriver::initialize( HWND hWnd )
{
  mhWnd = hWnd;

  auto iniPath = gConfigProvider.appDataFolder();

  try
  {
    std::tie( mBaseRenderer, mExtendedRenderer ) = DX11Renderer::create( hWnd, iniPath );
  }
  catch ( ... )
  {
    std::tie( mBaseRenderer, mExtendedRenderer ) = DX9Renderer::create( hWnd, iniPath );
  }

  mIntputSource = std::make_shared<UserInput>();
}

int SystemDriver::eventLoop()
{
  MSG msg{};

  for ( ;; )
  {
    while ( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
    {
      TranslateMessage( &msg );
      DispatchMessage( &msg );

      if ( msg.message == WM_QUIT )
        return (int)msg.wParam;
    }

    update();

    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
  }

  return 0;
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
  if ( mUpdateHandler )
    mUpdateHandler();

  if ( mPaused != mNewPaused )
  {
    mPaused = mNewPaused;

#ifdef FELIX86
    std::wstring n = L"Felix32 " + std::wstring{ version_string } + L" " + mImageFileName;
#else
    std::wstring n = L"Felix " + std::wstring{ version_string } + L" " + mImageFileName;
#endif

    if ( mPaused )
      n += L" paused";

    SetWindowText( mhWnd, n.c_str() );
  }
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

void SystemDriver::setImageName( std::wstring name )
{
  mImageFileName = std::move( name );
  std::wstring n = L"Felix " + std::wstring{ version_string } + L" " + mImageFileName;

  SetWindowText( mhWnd, n.c_str() );
}

void SystemDriver::setPaused( bool paused )
{
  mNewPaused = paused;
}

void SystemDriver::registerDropFiles( std::function<void( std::filesystem::path )> dropFilesHandler )
{
  mDropFilesHandler = std::move( dropFilesHandler );
}

void SystemDriver::registerUpdate( std::function<void()> updateHandler )
{
  mUpdateHandler = std::move( updateHandler );
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
    //removes null terminating character
    arg.pop_back();
  }

  DragFinish( hDrop );
  if ( mDropFilesHandler )
    mDropFilesHandler( std::move( arg ) );
}

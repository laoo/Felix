#include "pch.hpp"
#include "WinRenderer.hpp"
#include "Core.hpp"
#include "IInputSource.hpp"
#include "Log.hpp"
#include "WinAudioOut.hpp"
#include "InputFile.hpp"
#include "Manager.hpp"
#include "ComLynxWire.hpp"
#include "version.hpp"
#include "ConfigProvider.hpp"
#include "WinConfig.hpp"
#include "SysConfig.hpp"

wchar_t gClassName[] = L"FelixWindowClass";


LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch ( msg )
  {
  case WM_CREATE:
  {
    Manager* manager = reinterpret_cast<Manager*>( reinterpret_cast<LPCREATESTRUCT>( lParam )->lpCreateParams );
    assert( manager );
    try
    {
      manager->initialize( hwnd );
    }
    catch ( std::exception const & ex )
    {
      MessageBoxA( nullptr, ex.what(), "Renderinit Error", MB_OK | MB_ICONERROR );
      PostQuitMessage( 0 );
    }
    SetWindowLongPtr( hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( manager ) );
    break;
  }
  default:
    if ( Manager* manager = reinterpret_cast<Manager*>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) ) )
    {
      if ( manager->win32_WndProcHandler( hwnd, msg, wParam, lParam ) )
        return 1;
    }
    return DefWindowProc( hwnd, msg, wParam, lParam );
  }

  return 0;
}

int loop( Manager & manager )
{
  MSG msg{};

  while ( manager.doRun() )
  {
    while ( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
    {
      TranslateMessage( &msg );
      DispatchMessage( &msg );

      if ( msg.message == WM_QUIT )
        return (int)msg.wParam;
    }

    manager.update();

    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
  }

  return 0;
}

bool checkInstance( std::wstring const& name, std::wstring const& arg )
{
  if ( auto hwnd = FindWindowW( gClassName, name.c_str() ) )
  {
    std::vector<wchar_t> buffer;
    if ( !arg.empty() )
    {
      std::copy( arg.cbegin(), arg.cend(), std::back_inserter( buffer ) );
    }
    buffer.push_back( 0 );

    COPYDATASTRUCT cds;
    cds.dwData = 0;
    cds.cbData = (DWORD)buffer.size() * sizeof( wchar_t );
    cds.lpData = buffer.data();
    SendMessage( hwnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>( &cds ) );
    SetForegroundWindow( hwnd );
    return true;
  }
  
  return false;
}


std::wstring getCommandArg()
{
  LPWSTR* szArgList;
  int argCount;

  szArgList = CommandLineToArgvW( GetCommandLine(), &argCount );
  BOOST_SCOPE_EXIT_ALL( = )
  {
    LocalFree( szArgList );
  };

  if ( szArgList != NULL && argCount > 1 )
  {
    return std::wstring{ szArgList[1] };
  }

  return {};
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
  L_SET_LOGLEVEL( Log::LL_DEBUG );

  std::wstring arg = getCommandArg();

  Manager manager{};

  try
  {

    WNDCLASSEX wc{};

    wc.cbSize        = sizeof( WNDCLASSEX );
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon( NULL, IDI_APPLICATION );
    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = gClassName;
    wc.hIconSm       = LoadIcon( NULL, IDI_APPLICATION );

    if ( !RegisterClassEx( &wc ) )
    {
      return 0;
    }


    std::wstring name = L"Felix " + std::wstring{ version_string };

    auto winConfig = gConfigProvider.winConfig();

    if ( gConfigProvider.sysConfig()->singleInstance && checkInstance( name, arg ) )
      return 0;

    HWND hwnd = CreateWindowEx( WS_EX_CLIENTEDGE, gClassName, name.c_str(), WS_OVERLAPPEDWINDOW, winConfig->mainWindow.x, winConfig->mainWindow.y, winConfig->mainWindow.width, winConfig->mainWindow.height, nullptr, nullptr, hInstance, &manager );

    if ( hwnd == nullptr )
    {
      return 0;
    }

    ShowWindow( hwnd, nCmdShow );
    UpdateWindow( hwnd );
    DragAcceptFiles( hwnd, TRUE );

    manager.doArg( std::move( arg ) );

    return loop( manager );
  }
  catch ( sol::error const& err )
  {
    L_ERROR << err.what();
    MessageBoxA( nullptr, err.what(), "Error", 0 );
    return -1;
  }
  catch ( std::exception const& ex )
  {
    L_ERROR << ex.what();
    MessageBoxA( nullptr, ex.what(), "Error", 0 );
    return -1;
  }
}

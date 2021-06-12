#include "pch.hpp"
#include "WinRenderer.hpp"
#include "Felix.hpp"
#include "Mikey.hpp"
#include "KeyInput.hpp"
#include "Log.hpp"
#include "WinAudioOut.hpp"
#include "InputFile.hpp"
#include "version.hpp"

std::vector<std::wstring> gDroppedFiles;
KeyInput gKeyInput;


wchar_t gClassName[] = L"FelixWindowClass";


void handleFileDrop( HDROP hDrop )
{
#ifdef _WIN64
  auto h = GlobalAlloc( GMEM_MOVEABLE, 0 );
  uintptr_t hptr = reinterpret_cast<uintptr_t>( h );
  GlobalFree( h );
  uintptr_t hdropptr = reinterpret_cast<uintptr_t>( hDrop );
  hDrop = reinterpret_cast<HDROP>( hptr & 0xffffffff00000000 | hdropptr & 0xffffffff );
#endif

  uint32_t cnt = DragQueryFile( hDrop, ~0, nullptr, 0 );
  gDroppedFiles.resize( cnt );

  for ( uint32_t i = 0; i < cnt; ++i )
  {
    uint32_t size = DragQueryFile( hDrop, i, nullptr, 0 );
    gDroppedFiles[i].resize( size + 1 );
    DragQueryFile( hDrop, i, gDroppedFiles[i].data(), size + 1 );
  }

  DragFinish( hDrop );
}

LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch ( msg )
  {
  case WM_CREATE:
  {
    WinRenderer * pRenderer = reinterpret_cast<WinRenderer *>( reinterpret_cast<LPCREATESTRUCT>( lParam )->lpCreateParams );
    assert( pRenderer );
    try
    {
      pRenderer->initialize( hwnd );
    }
    catch ( std::exception const & ex )
    {
      MessageBoxA( nullptr, ex.what(), "Renderinit Error", MB_OK | MB_ICONERROR );
      PostQuitMessage( 0 );
    }
    SetWindowLongPtr( hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( pRenderer ) );
    return 0;
  }
  case WM_KEYDOWN:
  case WM_KEYUP:
    {
      WORD hi = HIWORD( lParam );
      bool down = ( hi & KF_UP ) == 0;
      bool nonRepeat = ( hi & KF_REPEAT ) == 0;

      if ( nonRepeat == down )
      {
        switch ( wParam )
        {
        case VK_LEFT:
          gKeyInput.left = down;
          break;
        case VK_UP:
          gKeyInput.up = down;
          break;
        case VK_RIGHT:
          gKeyInput.right = down;
          break;
        case VK_DOWN:
          gKeyInput.down = down;
          break;
        case '1':
          gKeyInput.opt1 = down;
          break;
        case '2':
          gKeyInput.opt2 = down;
          break;
        case 'Q':
          gKeyInput.pause = down;
          break;
        case 'Z':
          gKeyInput.a = down;
          break;
        case 'X':
          gKeyInput.b = down;
          break;
        default:
          break;
        }
      }
    }
    break;
  case WM_CLOSE:
    DestroyWindow( hwnd );
    return 0;
  case WM_DESTROY:
    PostQuitMessage( 0 );
    return 0;
  case WM_DROPFILES:
    handleFileDrop( (HDROP)wParam );
    return 0;
  default:
    break;
  }

  if ( WinRenderer * pRenderer = reinterpret_cast<WinRenderer *>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) ) )
  {
    if ( pRenderer->win32_WndProcHandler( hwnd, msg, wParam, lParam ) )
      return true;
  }
  return DefWindowProc( hwnd, msg, wParam, lParam );
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
  L_SET_LOGLEVEL( Log::LL_TRACE );

  std::vector<std::wstring> args;

  LPWSTR *szArgList;
  int argCount;

  szArgList = CommandLineToArgvW( GetCommandLine(), &argCount );
  if ( szArgList != NULL )
  {
    for ( int i = 1; i < argCount; i++ )
    {
      args.emplace_back( szArgList[i] );
    }

    LocalFree( szArgList );
  }

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

  std::thread renderThread;
  std::thread audioThread;
  std::atomic_bool doProcess = true;

  MSG msg;
  try
  {
    std::shared_ptr<WinRenderer> renderer = std::make_shared<WinRenderer>();
    std::shared_ptr<WinAudioOut> audioOut = std::make_shared<WinAudioOut>();
    std::shared_ptr<Felix> felix = std::make_shared<Felix>( renderer, [] { return gKeyInput; } );

    for ( auto const & arg : args )
    {
      {
        std::filesystem::path path{ arg };
        if ( path.has_extension() && path.extension() == ".log" )
        {
          felix->setLog( arg );
        }
      }

      InputFile file{ arg };
      if ( file.valid() )
      {
        felix->injectFile( file );
      }
    }

    std::wstring name = L"Felix " + std::wstring{ version_string };

    HWND hwnd = CreateWindowEx( WS_EX_CLIENTEDGE, gClassName, name.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 320*3, 210*3, nullptr, nullptr, hInstance, renderer.get() );

    if ( hwnd == nullptr )
    {
      return 0;
    }

    ShowWindow( hwnd, nCmdShow );
    UpdateWindow( hwnd );
    DragAcceptFiles( hwnd, TRUE );

    renderThread = std::thread{ [&,renderer]
    {
      while ( doProcess.load() )
      {
        renderer->render( felix );
      }
    } };

    audioThread = std::thread{ [&,audioOut,felix]
    {
      std::function<std::pair<float, float>( int sps )> sampleSource = [=]( int sps ) ->std::pair<float, float>
      {
        return felix->getSample( sps );
      };

      while ( doProcess.load() )
      {
        audioOut->fillBuffer( sampleSource );
      }
    } };
 
    while ( felix->running() )
    {
      while ( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
      {
        TranslateMessage( &msg );
        DispatchMessage( &msg );

        if ( msg.message == WM_QUIT )
          break;
      }

      if ( msg.message == WM_QUIT )
        break;


      if ( !gDroppedFiles.empty() )
      {
        for ( auto const& arg : gDroppedFiles )
        {
          InputFile file{ arg };
          if ( file.valid() )
          {
            felix->injectFile( file );
          }
        }

        gDroppedFiles.clear();
      }

      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
    }
  }
  catch ( std::runtime_error const& )
  {
    return -1;
  }

  doProcess.store( false );
  if ( audioThread.joinable() )
    audioThread.join();
  if ( renderThread.joinable() )
    renderThread.join();

  return (int)msg.wParam;
}

#include "pch.hpp"
#include "WinRenderer.hpp"
#include "Felix.hpp"
#include "Mikey.hpp"
#include "KeyInput.hpp"
#include "Log.hpp"
#include "WinAudioOut.hpp"
#include "InputFile.hpp"
#include "Config.hpp"
#include "ComLynxWire.hpp"
#include "version.hpp"

std::vector<std::wstring> gDroppedFiles;
KeyInput gKeyInput0;
KeyInput gKeyInput1;


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

void processKeys()
{
  std::array<uint8_t, 256> keys;
  if ( !GetKeyboardState( keys.data() ) )
    return;

  gKeyInput0.left = keys['A'] & 0x80;
  gKeyInput0.up = keys['W'] & 0x80;
  gKeyInput0.right = keys['D'] & 0x80;
  gKeyInput0.down = keys['S'] & 0x80;
  gKeyInput0.opt1 = keys['1'] & 0x80;
  gKeyInput0.pause = keys['2'] & 0x80;
  gKeyInput0.opt2 = keys['3'] & 0x80;
  gKeyInput0.a = keys[VK_LCONTROL] & 0x80;
  gKeyInput0.b = keys[VK_LSHIFT] & 0x80;

  gKeyInput1.left = keys[VK_LEFT] & 0x80;
  gKeyInput1.up = keys[VK_UP] & 0x80;
  gKeyInput1.right = keys[VK_RIGHT] & 0x80;
  gKeyInput1.down = keys[VK_DOWN] & 0x80;
  gKeyInput1.opt1 = keys[VK_DELETE] & 0x80;
  gKeyInput1.pause = keys[VK_END] & 0x80;
  gKeyInput1.opt2 = keys[VK_NEXT] & 0x80;
  gKeyInput1.a = keys[VK_RCONTROL] & 0x80;
  gKeyInput1.b = keys[VK_RSHIFT] & 0x80;
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
    std::shared_ptr<Config> config = std::make_shared<Config>();
    std::shared_ptr<WinRenderer> renderer = std::make_shared<WinRenderer>( 2 );
    std::shared_ptr<WinAudioOut> audioOut = std::make_shared<WinAudioOut>();
    std::shared_ptr<ComLynxWire> comLynxWire = std::make_shared<ComLynxWire>();

    std::vector<std::shared_ptr<Felix>> instances;

    instances.push_back( std::make_shared<Felix>( comLynxWire, renderer->getVideoSink( 0 ), [] { return gKeyInput0; } ) );
    instances.push_back( std::make_shared<Felix>( comLynxWire, renderer->getVideoSink( 1 ), [] { return gKeyInput1; } ) );

    for ( auto const & arg : args )
    {
      {
        std::filesystem::path path{ arg };
        if ( path.has_extension() && path.extension() == ".log" )
        {
          auto path1 = path;
          auto path2 = path;
          path1.replace_extension() += "[1]";
          path2.replace_extension() += "[2]";
          path1.replace_extension( ".log" );
          path2.replace_extension( ".log" );
          instances[0]->setLog( path1 );
          instances[1]->setLog( path2 );
        }
      }

      InputFile file{ arg };
      if ( file.valid() )
      {
        instances[0]->injectFile( file );
        instances[1]->injectFile( file );
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

    renderThread = std::thread{ [&doProcess,config,renderer]
    {
      while ( doProcess.load() )
      {
        renderer->render( *config );
      }
    } };

    audioThread = std::thread{ [&doProcess,audioOut,instances]
    {
      while ( doProcess.load() )
      {
        audioOut->fillBuffer( std::span<std::shared_ptr<Felix> const>{ instances.data(), instances.size() } );
      }
    } };
 
    while ( config->doRun() )
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

      processKeys();

      if ( !gDroppedFiles.empty() )
      {
        for ( auto const& arg : gDroppedFiles )
        {
          InputFile file{ arg };
          if ( file.valid() )
          {
            instances[0]->injectFile( file );
            instances[1]->injectFile( file );
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

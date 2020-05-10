#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <experimental/coroutine>
#include "WinRenderer.hpp"
#include "BusMaster.hpp"
#include "CPUExecute.hpp"
#include "CPUTrace.hpp"
#include "Mikey.hpp"


std::vector<std::wstring> gDroppedFiles;

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
  case WM_CLOSE:
    DestroyWindow( hwnd );
    break;
  case WM_DESTROY:
    PostQuitMessage( 0 );
    break;
  case WM_DROPFILES:
    handleFileDrop( (HDROP)wParam );
  default:
    return DefWindowProc( hwnd, msg, wParam, lParam );
  }
  return 0;
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
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

  HWND hwnd = CreateWindowEx( WS_EX_CLIENTEDGE, gClassName, L"Felix", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 320, 210, nullptr, nullptr, hInstance, nullptr );

  if ( hwnd == nullptr )
  {
    return 0;
  }

  MSG msg;
  try
  {
    WinRenderer renderer{ hwnd };

    ShowWindow( hwnd, nCmdShow );
    UpdateWindow( hwnd );

    DragAcceptFiles( hwnd, TRUE );


    CPU cpu;
    Mikey mikey;
    BusMaster bus{ mikey };
    CpuLoop loop = cpuLoop( cpu );
    //CpuTrace trace = cpuTrace( cpu, bus.getTraceRequest() );
    loop.setBusMaster( &bus );

    for ( ;; )
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

      //auto t1 = std::chrono::high_resolution_clock::now();
      bus.process( 16000000 / 60 );
      //auto t2 = std::chrono::high_resolution_clock::now();
      //auto diff = t2 - t1;
      //char buf[100];
      //sprintf( buf, "%llu\n", std::chrono::duration_cast< std::chrono::milliseconds >( diff ).count() );
      //OutputDebugStringA( buf );

      renderer.render( mikey.getSrface() );
    }
  }
  catch ( std::runtime_error const& )
  {
    return -1;
  }

  return (int)msg.wParam;

}
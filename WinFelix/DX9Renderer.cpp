#include "pch.hpp"
#include "DX9Renderer.hpp"
#include "ScreenRenderingBuffer.hpp"
#include "WinImgui9.hpp"
#include "Manager.hpp"
#include "VideoSink.hpp"

#define V_THROW(x) { HRESULT hr_ = (x); if( FAILED( hr_ ) ) { throw std::runtime_error{ "DXError" }; } }

DX9Renderer::DX9Renderer( HWND hWnd, std::filesystem::path const& iniPath ) : mHWnd{ hWnd }, mIniPath{ iniPath }, mVideoSink{ std::make_shared<VideoSink>() },
  mD3D{}, mD3Device{}, mRect{}, mSource{}, mSourceWidth{}, mSourceHeight{}, mLastRenderTimePoint{}
{
  LARGE_INTEGER l;
  QueryPerformanceCounter( &l );
  mLastRenderTimePoint = l.QuadPart;
}

DX9Renderer::~DX9Renderer()
{
}

std::shared_ptr<IExtendedRenderer> DX9Renderer::extendedRenderer()
{
  return {};
}

int64_t DX9Renderer::render( UI& ui )
{
  LARGE_INTEGER l;
  QueryPerformanceCounter( &l );

  internalRender( ui );
  mD3Device->PresentEx( nullptr, nullptr, nullptr, nullptr, 0 );

  auto result = l.QuadPart - mLastRenderTimePoint;
  mLastRenderTimePoint = l.QuadPart;
  return result;
}

void DX9Renderer::setRotation( ImageProperties::Rotation rotation )
{
  mRotation = rotation;
}

std::shared_ptr<IVideoSink> DX9Renderer::getVideoSink()
{
  return mVideoSink;
}

void DX9Renderer::internalRender( UI& ui )
{
  RECT r;
  if ( ::GetClientRect( mHWnd, &r ) == 0 )
    return;

  if ( mScreenGeometry.update( r.right, r.bottom, mRotation ) )
  {
    if ( !mD3Device )
    {
      typedef HRESULT( WINAPI* LPDIRECT3DCREATE9EX )( UINT SDKVersion, IDirect3D9Ex** );
      static LPDIRECT3DCREATE9EX s_Direct3DCreate9Ex = nullptr;
      HMODULE hModD3D9 = ::LoadLibrary( L"d3d9.dll" );
      if ( hModD3D9 == nullptr )
        throw std::runtime_error{ "DXError" };

      s_Direct3DCreate9Ex = (LPDIRECT3DCREATE9EX)GetProcAddress( hModD3D9, "Direct3DCreate9Ex" );

      V_THROW( s_Direct3DCreate9Ex( D3D_SDK_VERSION, mD3D.ReleaseAndGetAddressOf() ) );

      D3DPRESENT_PARAMETERS presentParams;
      ZeroMemory( &presentParams, sizeof( presentParams ) );
      presentParams.Windowed = TRUE;
      presentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
      presentParams.BackBufferFormat = D3DFMT_X8R8G8B8;
      presentParams.BackBufferCount = 1;
      presentParams.BackBufferWidth = 256;
      presentParams.BackBufferHeight = 256;
      presentParams.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
      presentParams.Flags = D3DPRESENTFLAG_VIDEO;

      V_THROW( mD3D->CreateDeviceEx( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED, &presentParams, nullptr, mD3Device.ReleaseAndGetAddressOf() ) );

      mImgui = std::make_shared<WinImgui9>( mHWnd, mD3Device, mIniPath );
    }


    D3DPRESENT_PARAMETERS presentParams;
    ZeroMemory( &presentParams, sizeof( presentParams ) );
    presentParams.Windowed = TRUE;
    presentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
    presentParams.BackBufferFormat = D3DFMT_X8R8G8B8;
    presentParams.EnableAutoDepthStencil = false;
    presentParams.BackBufferCount = 1;
    presentParams.hDeviceWindow = mHWnd;
    presentParams.BackBufferWidth = mScreenGeometry.windowWidth();
    presentParams.BackBufferHeight = mScreenGeometry.windowHeight();
    presentParams.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    V_THROW( mD3Device->ResetEx( &presentParams, nullptr ) );

    mRect = RECT{ mScreenGeometry.xOff(), mScreenGeometry.yOff(),  mScreenGeometry.xOff() + mScreenGeometry.width(), mScreenGeometry.yOff() + mScreenGeometry.height() };
  }

  if ( !mScreenGeometry )
    return;

  ComPtr<IDirect3DSurface9> rtSurface;
  V_THROW( mD3Device->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, rtSurface.ReleaseAndGetAddressOf() ) );
  V_THROW( mD3Device->ColorFill( rtSurface.Get(), NULL, D3DCOLOR_XRGB( 255, 255, 255 ) ) );

  if ( !mSource )
  {
    V_THROW( mD3Device->CreateTexture( SCREEN_WIDTH, SCREEN_HEIGHT, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, mSource.ReleaseAndGetAddressOf(), nullptr ) );
    mTempBuffer.resize( SCREEN_WIDTH * SCREEN_HEIGHT / 2, ~0ull );
  }

  {
    if ( auto frame = mVideoSink->pullNextFrame() )
    {
      struct MappedTexture
      {
        VideoSink::DPixel* data;
        uint32_t stride;
      } map{ ( VideoSink::DPixel *)mTempBuffer.data(), (uint32_t)SCREEN_WIDTH / 2 };

      for ( int i = 0; i < (int)ScreenRenderingBuffer::ROWS_COUNT; ++i )
      {
        auto const& row = frame->row(i);
        int size = frame->size(i);
        VideoSink::DPixel* dst = map.data + std::max( 0, ( i - 3 ) ) * map.stride;

        for ( int j = 0; j < size; ++j )
        {
          uint16_t v = row[j];
          if ( std::bit_cast<int16_t>( v ) < 0 )
          {
            *dst++ = mVideoSink->mPalette[(uint8_t)v];
          }
          else
          {
            mVideoSink->updatePalette( v >> 8, (uint8_t)v );
          }
        }
      }
    }

    ComPtr<IDirect3DTexture9> sysTexture;
    HANDLE h = (HANDLE)mTempBuffer.data();

    V_THROW( mD3Device->CreateTexture( SCREEN_WIDTH, SCREEN_HEIGHT, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, sysTexture.ReleaseAndGetAddressOf(), &h ) );
    V_THROW( mD3Device->UpdateTexture( sysTexture.Get(), mSource.Get() ) );

    ComPtr<IDirect3DSurface9> srcSurface;
    mSource->GetSurfaceLevel( 0, srcSurface.ReleaseAndGetAddressOf() );
    V_THROW( mD3Device->StretchRect( srcSurface.Get(), nullptr, rtSurface.Get(), &mRect, D3DTEXF_POINT ) );
  }

  GetWindowRect( mHWnd, &r );
  POINT p{ r.left, r.top };
  ScreenToClient( mHWnd, &p );
  GetClientRect( mHWnd, &r );
  r.left = p.x;
  r.top = p.y;

  mImgui->newFrame();

  ImGui::NewFrame();

  ui.drawGui( r.left, r.top, r.right, r.bottom );


  V_THROW( mD3Device->BeginScene() );

  ImGui::Render();
  mImgui->renderDrawData( ImGui::GetDrawData() );

  V_THROW( mD3Device->EndScene() );
}

int DX9Renderer::sizing( RECT& rect )
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

int DX9Renderer::wndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
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

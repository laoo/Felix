#include "pch.hpp"
#include "DX9Renderer.hpp"
#include "ScreenRenderingBuffer.hpp"
#include "WinImgui9.hpp"
#include "Manager.hpp"
#include "VideoSink.hpp"

#define V_THROW(x) { HRESULT hr_ = (x); if( FAILED( hr_ ) ) { throw std::runtime_error{ "DXError" }; } }

DX9Renderer::DX9Renderer( HWND hWnd, std::filesystem::path const& iniPath ) : BaseRenderer{ hWnd }, mIniPath{ iniPath }, mImgui{}, mD3D{}, mD3Device{}, mRect{}, mSource{}, mSourceWidth{}, mSourceHeight{}
{
}

void DX9Renderer::present()
{
  mD3Device->PresentEx( nullptr, nullptr, nullptr, nullptr, 0 );
}

void DX9Renderer::internalRender( Manager& config )
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

  RECT r;
  if ( ::GetClientRect( mHWnd, &r ) == 0 )
    return;

  if ( mScreenGeometry.windowHeight() != r.bottom || ( mScreenGeometry.windowWidth() != r.right ) || mScreenGeometry.rotation() != mRotation )
  {
    mScreenGeometry = ScreenGeometry{ r.right, r.bottom, mRotation };

    if ( !mScreenGeometry )
    {
      return;
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

    mRect = RECT{ mScreenGeometry.xOff(), mScreenGeometry.yOff(),  mScreenGeometry.xOff() + mScreenGeometry.width() * mScreenGeometry.scale(), mScreenGeometry.yOff() + mScreenGeometry.height() * mScreenGeometry.scale() };

  }
  

  ComPtr<IDirect3DSurface9> rtSurface;
  V_THROW( mD3Device->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, rtSurface.ReleaseAndGetAddressOf() ) );
  V_THROW( mD3Device->ColorFill( rtSurface.Get(), NULL, D3DCOLOR_XRGB( 255, 255, 255 ) ) );

  if ( mSourceWidth != mScreenGeometry.width() || mSourceHeight != mScreenGeometry.height() )
  {
    V_THROW( mD3Device->CreateTexture( mScreenGeometry.width(), mScreenGeometry.height(), 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, mSource.ReleaseAndGetAddressOf(), nullptr ) );
    mSourceWidth = mScreenGeometry.width();
    mSourceHeight = mScreenGeometry.height();
    mTempBuffer.resize( mSourceHeight * mSourceWidth / 2, ~0ull );
  }

  {
    if ( auto frame = mVideoSink->pullNextFrame() )
    {
      struct MappedTexture
      {
        VideoSink::DPixel* data;
        uint32_t stride;
      } map{ ( VideoSink::DPixel *)mTempBuffer.data(), (uint32_t)mSourceWidth / 2 };

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

    V_THROW( mD3Device->CreateTexture( mScreenGeometry.width(), mScreenGeometry.height(), 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, sysTexture.ReleaseAndGetAddressOf(), &h ) );
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

  mImgui->dx9_NewFrame();
  mImgui->win32_NewFrame();

  ImGui::NewFrame();

  config.drawGui( r.left, r.top, r.right, r.bottom );


  V_THROW( mD3Device->BeginScene() );

  ImGui::Render();
  mImgui->dx9_RenderDrawData( ImGui::GetDrawData() );

  V_THROW( mD3Device->EndScene() );
}

void DX9Renderer::setEncoder( std::shared_ptr<IEncoder> encoder )
{
}

int DX9Renderer::win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
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


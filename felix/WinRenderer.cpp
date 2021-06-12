#include "pch.hpp"
#include "WinRenderer.hpp"
#include "Felix.hpp"
#include "imgui.h"
#include "WinImgui.hpp"
#include "renderer.hxx"
#include "Log.hpp"

#define V_THROW(x) { HRESULT hr_ = (x); if( FAILED( hr_ ) ) { throw std::runtime_error{ "DXError" }; } }


struct RenderFrame
{
  static constexpr size_t DISPLAY_BYTES = 80;
  static constexpr size_t MAX_COLOR_CHANGES = 256;
  static constexpr size_t LINE_BUFFER_SIZE = DISPLAY_BYTES + MAX_COLOR_CHANGES;

  RenderFrame() : idx{}, id{ sId++ } {}

  uint64_t idx;
  uint32_t id;
  std::array<uint16_t, LINE_BUFFER_SIZE * 102> frameBuffer;

  void pushColorChage( uint8_t reg, uint8_t value )
  {
    frameBuffer[idx++] = ( reg << 8 ) | value;
  }
  void pushScreenBytes( std::span<uint8_t const> data )
  {
    for ( uint8_t byte : data )
    {
      frameBuffer[idx++] = 0xff00 | (uint16_t)byte;
    }
  }

  static uint32_t sId;
};

uint32_t RenderFrame::sId = 0;

WinRenderer::WinRenderer() : mActiveFrame{}, mFinishedFrames{}, mQueueMutex{}, mIdx{}, mHWnd{}, theWinWidth{}, theWinHeight{}, mRefreshRate{}, mPerfCount{}, mFrameTicks{ ~0ull }
{
  QueryPerformanceFrequency( (LARGE_INTEGER*)&mPerfFreq );

  for ( uint32_t i = 0; i < 256; ++i )
  {
    mPalette[i] = DPixel{ Pixel{ 0, 0, 0, 255 }, Pixel{ 0, 0, 0, 255 } };
  }
}


void WinRenderer::initialize( HWND hWnd )
{
  mHWnd = hWnd;

  typedef HRESULT( WINAPI * LPD3D11CREATEDEVICE )( IDXGIAdapter *, D3D_DRIVER_TYPE, HMODULE, UINT32, CONST D3D_FEATURE_LEVEL *, UINT, UINT32, ID3D11Device **, D3D_FEATURE_LEVEL *, ID3D11DeviceContext ** );
  static LPD3D11CREATEDEVICE  s_DynamicD3D11CreateDevice = nullptr;
  HMODULE hModD3D11 = ::LoadLibrary( L"d3d11.dll" );
  if ( hModD3D11 == nullptr )
    throw std::runtime_error{ "DXError" };

  s_DynamicD3D11CreateDevice = (LPD3D11CREATEDEVICE)GetProcAddress( hModD3D11, "D3D11CreateDevice" );


  D3D_FEATURE_LEVEL  featureLevelsRequested = D3D_FEATURE_LEVEL_11_0;
  UINT               numFeatureLevelsRequested = 1;
  D3D_FEATURE_LEVEL  featureLevelsSupported;

  HRESULT hr = s_DynamicD3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
#ifndef NDEBUG
    D3D11_CREATE_DEVICE_DEBUG,
#else
    0,
#endif
    & featureLevelsRequested, numFeatureLevelsRequested, D3D11_SDK_VERSION, mD3DDevice.ReleaseAndGetAddressOf(), &featureLevelsSupported, mImmediateContext.ReleaseAndGetAddressOf() );

  V_THROW( hr );

  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory( &sd, sizeof( sd ) );
  sd.BufferCount = 2;
  sd.BufferDesc.Width = 0;
  sd.BufferDesc.Height = 0;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.RefreshRate.Numerator = 0;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.BufferUsage = DXGI_USAGE_UNORDERED_ACCESS | DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = mHWnd;
  sd.SampleDesc.Count = 1;
  sd.SampleDesc.Quality = 0;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

  ComPtr<IDXGIDevice> pDXGIDevice;
  V_THROW( mD3DDevice.As( &pDXGIDevice ) );

  ComPtr<IDXGIAdapter> pDXGIAdapter;
  V_THROW( pDXGIDevice->GetAdapter( pDXGIAdapter.ReleaseAndGetAddressOf() ) );

  ComPtr<IDXGIFactory> pIDXGIFactory;
  V_THROW( pDXGIAdapter->GetParent( __uuidof( IDXGIFactory ), (void **)pIDXGIFactory.ReleaseAndGetAddressOf() ) );

  V_THROW( pIDXGIFactory->CreateSwapChain( mD3DDevice.Get(), &sd, mSwapChain.ReleaseAndGetAddressOf() ) );

  ComPtr<IDXGIOutput> pIDXGIOutput;
  V_THROW( mSwapChain->GetContainingOutput( pIDXGIOutput.ReleaseAndGetAddressOf() ) );

  DXGI_MODE_DESC md;
  V_THROW( pIDXGIOutput->FindClosestMatchingMode( &sd.BufferDesc, &md, mD3DDevice.Get() ) );
  mRefreshRate = { md.RefreshRate.Numerator, md.RefreshRate.Denominator };

  L_INFO << "Refresh Rate: " << mRefreshRate << " = " << ( (double)mRefreshRate.numerator() / (double)mRefreshRate.denominator() );

  V_THROW( mD3DDevice->CreateComputeShader( g_Renderer, sizeof g_Renderer, nullptr, mRendererCS.ReleaseAndGetAddressOf() ) );

  D3D11_BUFFER_DESC bd = {};
  bd.ByteWidth = sizeof( CBPosSize );
  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  V_THROW( mD3DDevice->CreateBuffer( &bd, NULL, mPosSizeCB.ReleaseAndGetAddressOf() ) );

  D3D11_TEXTURE2D_DESC desc{};
  desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.Width = 160;
  desc.Height = 102;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Usage = D3D11_USAGE_DYNAMIC;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  desc.MiscFlags = 0;
  mD3DDevice->CreateTexture2D( &desc, nullptr, mSource.ReleaseAndGetAddressOf() );
  V_THROW( mD3DDevice->CreateShaderResourceView( mSource.Get(), NULL, mSourceSRV.ReleaseAndGetAddressOf() ) );

  mImgui.reset( new WinImgui{ mHWnd, mD3DDevice, mImmediateContext } );
}

void WinRenderer::render( Felix & felix )
{
  if ( auto frame = pullNextFrame() )
  {
    updateSourceTexture( std::move( frame ) );
  }

  RECT r;
  if ( ::GetClientRect( mHWnd, &r ) == 0 )
    return;

  if ( theWinHeight != ( r.bottom - r.top ) || ( theWinWidth != r.right - r.left ) )
  {
    theWinHeight = r.bottom - r.top;
    theWinWidth = r.right - r.left;

    if ( theWinHeight == 0 || theWinWidth == 0 )
    {
      return;
    }

    mBackBufferUAV.Reset();
    mBackBufferRTV.Reset();

    mSwapChain->ResizeBuffers( 0, theWinWidth, theWinHeight, DXGI_FORMAT_UNKNOWN, 0 );

    ComPtr<ID3D11Texture2D> backBuffer;
    V_THROW( mSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)backBuffer.ReleaseAndGetAddressOf() ) );
    V_THROW( mD3DDevice->CreateUnorderedAccessView( backBuffer.Get(), nullptr, mBackBufferUAV.ReleaseAndGetAddressOf() ) );
    V_THROW( mD3DDevice->CreateRenderTargetView( backBuffer.Get(), nullptr, mBackBufferRTV.ReleaseAndGetAddressOf() ) );

    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)theWinWidth;
    vp.Height = (FLOAT)theWinHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    mImmediateContext->RSSetViewports( 1, &vp );
  }

  int sx = (std::max)( 1, theWinWidth / 160 );
  int sy = (std::max)( 1, theWinHeight / 102 );
  int s = (std::min)( sx, sy );

  CBPosSize cbPosSize{ ( theWinWidth - 160 * s ) / 2, ( theWinHeight - 102 * s ) / 2, s, s };
  mImmediateContext->UpdateSubresource( mPosSizeCB.Get(), 0, NULL, &cbPosSize, 0, 0 );
  mImmediateContext->CSSetConstantBuffers( 0, 1, mPosSizeCB.GetAddressOf() );
  mImmediateContext->CSSetShaderResources( 0, 1, mSourceSRV.GetAddressOf() );
  mImmediateContext->CSSetUnorderedAccessViews( 0, 1, mBackBufferUAV.GetAddressOf(), nullptr );
  mImmediateContext->CSSetShader( mRendererCS.Get(), nullptr, 0 );
  UINT v[4]= { 255, 255, 255, 255 };
  mImmediateContext->ClearUnorderedAccessViewUint( mBackBufferUAV.Get(), v );
  mImmediateContext->Dispatch( 10, 102, 1 );
  std::array<ID3D11UnorderedAccessView * const, 1> uav{};
  mImmediateContext->CSSetUnorderedAccessViews( 0, 1, uav.data(), nullptr );

  {
    RECT r;
    GetWindowRect( mHWnd, &r );
    POINT p{ r.left, r.top };
    ScreenToClient( mHWnd, &p );
    GetClientRect( mHWnd, &r );
    r.left = p.x;
    r.top = p.y;

    mImgui->dx11_NewFrame();
    mImgui->win32_NewFrame();

    ImGui::NewFrame();

    felix.drawGui( r.left, r.top, r.right, r.bottom );

    ImGui::Render();

    mImmediateContext->OMSetRenderTargets( 1, mBackBufferRTV.GetAddressOf(), nullptr );
    mImgui->dx11_RenderDrawData( ImGui::GetDrawData() );

    std::array<ID3D11RenderTargetView * const, 1> rtv{};
    mImmediateContext->OMSetRenderTargets( 1, rtv.data(), nullptr );
  }

  mSwapChain->Present( 1, 0 );
  int64_t cnt = 0;
  QueryPerformanceCounter( (LARGE_INTEGER*)&cnt );
  int64_t diff = cnt - mPerfCount;
  //L_TRACE << diff << "\t" << (double)mPerfFreq / (double)diff;
  mPerfCount = cnt;
}

void WinRenderer::startNewFrame( uint64_t tick )
{
  mBeginTick = tick;
  mLastTick = tick;
  mIdx = 0;
  if ( mActiveFrame )
  {
    mActiveFrame.reset();
  }
  mActiveFrame = std::make_shared<RenderFrame>();
}

void WinRenderer::endFrame( uint64_t tick )
{
  mLastTick = tick;
  mFrameTicks = tick - mBeginTick;
  if ( mActiveFrame )
  {
    std::scoped_lock<std::mutex> lock( mQueueMutex );
    if ( mFinishedFrames.size() > 1 )
    {
      mFinishedFrames.pop();
    }
    mFinishedFrames.push( std::move( mActiveFrame ) );
    mActiveFrame.reset();
  }
}

void WinRenderer::updatePalette( uint16_t reg, uint8_t value )
{
  reg &= 0xff;

  if ( reg < 16 )
  {
    uint32_t regLo = reg;
    uint32_t regHi = reg << 4;

    //green
    uint8_t g = ( value << 4 ) | ( value & 0x0f );

    for ( uint32_t i = regHi; i < regHi + 16; ++i )
    {
      mPalette[i].left.g = g;
    }
    for ( uint32_t i = regLo; i < 256; i += 16 )
    {
      mPalette[i].right.g = g;
    }
  }
  else
  {
    uint32_t regLo = reg & 0x0f;
    uint32_t regHi = regLo << 4;

    //blue
    uint8_t b = ( value >> 4 ) | ( value & 0xf0 );
    //red
    uint8_t r = ( value << 4 ) | ( value & 0x0f );

    for ( uint32_t i = regHi; i < regHi + 16; ++i )
    {
      mPalette[i].left.b = b;
      mPalette[i].left.r = r;
    }
    for ( uint32_t i = regLo; i < 256; i += 16 )
    {
      mPalette[i].right.b = b;
      mPalette[i].right.r = r;
    }
  }
}

void WinRenderer::updateSourceTexture( std::shared_ptr<RenderFrame> frame )
{
  assert( frame );
  D3D11_MAPPED_SUBRESOURCE map;
  mImmediateContext->Map( mSource.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map );
  DPixel * dst = (DPixel *)map.pData;
  assert( map.RowPitch / sizeof( DPixel ) == 80 );
  for ( size_t i = 0; i < frame->idx; ++i )
  {
    uint16_t v = frame->frameBuffer[i];
    if ( std::bit_cast<int16_t>( v ) < 0 )
    {
      *dst++ = mPalette[(uint8_t)v];
    }
    else
    {
      updatePalette( v >> 8, (uint8_t)v );
    }
  }
  mImmediateContext->Unmap( mSource.Get(), 0 );
}

std::shared_ptr<RenderFrame> WinRenderer::pullNextFrame()
{
  std::shared_ptr<RenderFrame> result{};

  std::scoped_lock<std::mutex> lock( mQueueMutex );
  if ( !mFinishedFrames.empty() )
  {
    result = mFinishedFrames.front();
    mFinishedFrames.pop();
  }

  return result;
}

bool WinRenderer::win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch ( msg )
  {
  case WM_SIZING:
    return sizing( *(RECT *)lParam );
  case WM_SIZE:
    break;
  default:
    return mImgui->win32_WndProcHandler( hWnd, msg, wParam, lParam );
  }

  return false;
}

bool WinRenderer::sizing( RECT & rect )
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

  if ( cW < 160 )
  {
    rect.left = wRect.left;
    rect.right = wRect.right;
  }
  if ( cH < 102 )
  {
    rect.top = wRect.top;
    rect.bottom = wRect.bottom;
  }

  return true;
}

void WinRenderer::emitScreenData( std::span<uint8_t const> data )
{
  if ( mActiveFrame )
    mActiveFrame->pushScreenBytes( data );
}

void WinRenderer::updateColorReg( uint8_t reg, uint8_t value )
{
  if ( mActiveFrame )
  {
    mActiveFrame->pushColorChage( reg, value );
  }
  else
  {
    updatePalette( reg, value );
  }
}


#include "pch.hpp"
#include "WinRenderer.hpp"
#include "Config.hpp"
#include "imgui.h"
#include "WinImgui.hpp"
#include "renderer.hxx"
#include "Log.hpp"

#define V_THROW(x) { HRESULT hr_ = (x); if( FAILED( hr_ ) ) { throw std::runtime_error{ "DXError" }; } }


struct RenderFrame
{
  static constexpr size_t DISPLAY_BYTES = 80;
  static constexpr size_t MAX_COLOR_CHANGES = 256;
  static constexpr size_t LINE_BUFFER_SIZE = 512;
  static_assert( DISPLAY_BYTES + MAX_COLOR_CHANGES <= LINE_BUFFER_SIZE );

  struct Row
  {
    std::array<uint16_t, LINE_BUFFER_SIZE> lineBuffer;
  };

  RenderFrame() : id{ sId++ }, currentRow{} {}

  std::array<int, 105> sizes;
  std::array<Row, 105> rows;
  uint32_t id;
  Row * currentRow;
  int * size;

  void newRow( int row )
  {
    int idx = 104 - std::clamp( row, 0, 104 );
    currentRow = &rows[ idx ];
    size = &sizes[ idx ];
    *size = 0;
  }

  void pushColorChage( uint8_t reg, uint8_t value )
  {
    int idx = *size;
    currentRow->lineBuffer[idx++] = ( reg << 8 ) | value;
    
    *size = idx & ( LINE_BUFFER_SIZE - 1 );
  }

  void pushScreenBytes( std::span<uint8_t const> data )
  {
    int idx = *size;
    for ( uint8_t byte : data )
    {
      currentRow->lineBuffer[idx++] = 0xff00 | (uint16_t)byte;
    }
    *size = idx & ( LINE_BUFFER_SIZE - 1 );
  }

  static uint32_t sId;
};

uint32_t RenderFrame::sId = 0;

WinRenderer::WinRenderer( int instances ) : mInstances{}, mHWnd{}, mSizeManager{}, mRefreshRate{}
{
  if ( instances <= 0 || instances > 2 )
    throw std::runtime_error{ "Bad emulation instances numer" };

  for ( int i = 0; i < instances; ++i )
  {
    mInstances.push_back( std::make_shared<Instance>() );
    mSources.push_back( {} );
    mSourceSRVs.push_back( {} );
  }
}

WinRenderer::~WinRenderer()
{
}

void WinRenderer::initialize( HWND hWnd )
{
  mHWnd = hWnd;

  typedef HRESULT( WINAPI * LPD3D11CREATEDEVICE )( IDXGIAdapter *, D3D_DRIVER_TYPE, HMODULE, UINT32, CONST D3D_FEATURE_LEVEL *, UINT, UINT32, ID3D11Device **, D3D_FEATURE_LEVEL *, ID3D11DeviceContext ** );
  static LPD3D11CREATEDEVICE s_DynamicD3D11CreateDevice = nullptr;
  HMODULE hModD3D11 = ::LoadLibrary( L"d3d11.dll" );
  if ( hModD3D11 == nullptr )
    throw std::runtime_error{ "DXError" };

  s_DynamicD3D11CreateDevice = (LPD3D11CREATEDEVICE)GetProcAddress( hModD3D11, "D3D11CreateDevice" );


  D3D_FEATURE_LEVEL  featureLevelsRequested = D3D_FEATURE_LEVEL_11_0;
  D3D_FEATURE_LEVEL  featureLevelsSupported;

  HRESULT hr = s_DynamicD3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
#ifndef NDEBUG
    D3D11_CREATE_DEVICE_DEBUG,
#else
    0,
#endif
    &featureLevelsRequested, 1, D3D11_SDK_VERSION, mD3DDevice.ReleaseAndGetAddressOf(), &featureLevelsSupported, mImmediateContext.ReleaseAndGetAddressOf() );

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

  for ( size_t i = 0; i < mInstances.size(); ++i )
  {
    mD3DDevice->CreateTexture2D( &desc, nullptr, mSources[i].ReleaseAndGetAddressOf() );
    V_THROW( mD3DDevice->CreateShaderResourceView( mSources[i].Get(), NULL, mSourceSRVs[i].ReleaseAndGetAddressOf() ) );
  }


  mImgui.reset( new WinImgui{ mHWnd, mD3DDevice, mImmediateContext } );
}

std::shared_ptr<IVideoSink> WinRenderer::getVideoSink( int instance ) const
{
  if ( instance >= 0 && instance < mInstances.size() )
    return mInstances[instance];
  else
    return {};
}

void WinRenderer::render( Config & config )
{
  RECT r;
  if ( ::GetClientRect( mHWnd, &r ) == 0 )
    return;

  if ( mSizeManager.windowHeight() != r.bottom || ( mSizeManager.windowWidth() != r.right ) )
  {
    mSizeManager = SizeManager{ (int)mInstances.size(), config.horizontalView(), r.right, r.bottom };

    if ( !mSizeManager )
    {
      return;
    }

    mBackBufferUAV.Reset();
    mBackBufferRTV.Reset();

    mSwapChain->ResizeBuffers( 0, mSizeManager.windowWidth(), mSizeManager.windowHeight(), DXGI_FORMAT_UNKNOWN, 0 );

    ComPtr<ID3D11Texture2D> backBuffer;
    V_THROW( mSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)backBuffer.ReleaseAndGetAddressOf() ) );
    V_THROW( mD3DDevice->CreateUnorderedAccessView( backBuffer.Get(), nullptr, mBackBufferUAV.ReleaseAndGetAddressOf() ) );
    V_THROW( mD3DDevice->CreateRenderTargetView( backBuffer.Get(), nullptr, mBackBufferRTV.ReleaseAndGetAddressOf() ) );
  }

  if ( config.horizontalView() != mSizeManager.horizontal() )
  {
    if ( r.right >= mSizeManager.minWindowWidth( config.horizontalView() ) && r.bottom >= mSizeManager.minWindowHeight( config.horizontalView() ) )
    {
      mSizeManager = SizeManager{ (int)mInstances.size(), config.horizontalView(), r.right, r.bottom };
    }
    else
    {
      config.horizontalView( mSizeManager.horizontal() );
    }
  }

  UINT v[4] = { 255, 255, 255, 255 };
  mImmediateContext->ClearUnorderedAccessViewUint( mBackBufferUAV.Get(), v );
  mImmediateContext->CSSetUnorderedAccessViews( 0, 1, mBackBufferUAV.GetAddressOf(), nullptr );

  for ( int i = 0; i < mInstances.size(); ++i )
  {
    if ( auto frame = mInstances[i]->pullNextFrame() )
    {
      updateSourceTexture( i, std::move( frame ) );
    }

    CBPosSize cbPosSize{ mSizeManager.instanceXOff( i ), mSizeManager.instanceYOff( i ), mSizeManager.scale() };
    mImmediateContext->UpdateSubresource( mPosSizeCB.Get(), 0, NULL, &cbPosSize, 0, 0 );
    mImmediateContext->CSSetConstantBuffers( 0, 1, mPosSizeCB.GetAddressOf() );
    mImmediateContext->CSSetShaderResources( 0, 1, mSourceSRVs[i].GetAddressOf() );
    mImmediateContext->CSSetShader( mRendererCS.Get(), nullptr, 0 );
    mImmediateContext->Dispatch( 5, 51, 1 );
  }


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

    config.drawGui( r.left, r.top, r.right, r.bottom );

    ImGui::Render();

    mImmediateContext->OMSetRenderTargets( 1, mBackBufferRTV.GetAddressOf(), nullptr );
    mImgui->dx11_RenderDrawData( ImGui::GetDrawData() );

    std::array<ID3D11RenderTargetView * const, 1> rtv{};
    mImmediateContext->OMSetRenderTargets( 1, rtv.data(), nullptr );
  }

  mSwapChain->Present( 1, 0 );
}

void WinRenderer::Instance::newFrame( uint64_t tick )
{
  mFrameTicks = tick - mBeginTick;
  mBeginTick = tick;
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
  mActiveFrame = std::make_shared<RenderFrame>();
}

void WinRenderer::Instance::newRow( uint64_t tick, int row )
{
  mActiveFrame->newRow( row );
}

void WinRenderer::Instance::updatePalette( uint16_t reg, uint8_t value )
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

void WinRenderer::updateSourceTexture( int instance, std::shared_ptr<RenderFrame> frame )
{
  assert( frame && instance >= 0 && instance < mInstances.size() );

  auto & inst = mInstances[instance];

  D3D11_MAPPED_SUBRESOURCE map;
  mImmediateContext->Map( mSources[instance].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map );
  size_t pitch = map.RowPitch / sizeof( DPixel );

  for ( int i = 0; i < (int)frame->rows.size(); ++i )
  {
    auto const &row = frame->rows[i];
    int size = frame->sizes[i];
    DPixel * dst = (DPixel *)map.pData + std::max( 0, ( i - 3 ) ) * pitch;

    for ( int j = 0; j < size; ++j )
    {
      uint16_t v = row.lineBuffer[j];
      if ( std::bit_cast<int16_t>( v ) < 0 )
      {
        *dst++ = inst->mPalette[(uint8_t)v];
      }
      else
      {
        inst->updatePalette( v >> 8, (uint8_t)v );
      }
    }
  }

  mImmediateContext->Unmap( mSources[instance].Get(), 0 );
}

std::shared_ptr<RenderFrame> WinRenderer::Instance::pullNextFrame()
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

  if ( cW < mSizeManager.minWindowWidth() )
  {
    rect.left = wRect.left;
    rect.right = wRect.right;
  }
  if ( cH < mSizeManager.minWindowHeight() )
  {
    rect.top = wRect.top;
    rect.bottom = wRect.bottom;
  }

  return true;
}

void WinRenderer::Instance::emitScreenData( std::span<uint8_t const> data )
{
  if ( mActiveFrame )
    mActiveFrame->pushScreenBytes( data );
}

void WinRenderer::Instance::updateColorReg( uint8_t reg, uint8_t value )
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

WinRenderer::SizeManager::SizeManager() : mWinWidth{}, mWinHeight{}, mScale{ 1 }, mInstances{ 1 }, mHorizontal{ true }
{
}

WinRenderer::SizeManager::SizeManager( int instances, bool horizontal, int windowWidth, int windowHeight ) : mWinWidth{ windowWidth }, mWinHeight{ windowHeight }, mScale{ 1 }, mInstances{ instances }, mHorizontal{ horizontal }
{
  if ( mInstances < 1 || mInstances > 2 )
    throw std::runtime_error{ "Bad emulation instances numer" };

  int sx, sy;
  switch ( mInstances )
  {
  case 2:
    if ( mHorizontal )
    {
      sx = (std::max)( 1, mWinWidth / 2 / 160 );
      sy = (std::max)( 1, mWinHeight / 102 );
    }
    else
    {
      sx = (std::max)( 1, mWinWidth / 160 );
      sy = (std::max)( 1, mWinHeight / 2 / 102 );
    }
    mScale = (std::min)( sx, sy );
    break;
  default:
    sx = (std::max)( 1, mWinWidth / 160 );
    sy = (std::max)( 1, mWinHeight / 102 );
    mScale = (std::min)( sx, sy );
    break;
  }
}

int WinRenderer::SizeManager::windowWidth() const
{
  return mWinWidth;
}

int WinRenderer::SizeManager::windowHeight() const
{
  return mWinHeight;
}

int WinRenderer::SizeManager::minWindowWidth( std::optional<bool> horizontal ) const
{
  switch ( mInstances )
  {
  case 2:
    if ( horizontal.value_or( mHorizontal ) )
    {
      return 160 * 2;
    }
    else
    {
      return 160;
    }
  default:
    return 160;
  }
}

int WinRenderer::SizeManager::minWindowHeight( std::optional<bool> horizontal ) const
{
  switch ( mInstances )
  {
  case 2:
    if ( horizontal.value_or( mHorizontal ) )
    {
      return 102;
    }
    else
    {
      return 102 * 2;
    }
  default:
    return 102;
  }
}

int WinRenderer::SizeManager::instanceXOff( int instance ) const
{
  if ( mInstances == 2 && mHorizontal )
  {
    return ( ( instance == 1 ? 3 : 1 ) * mWinWidth - 320 * mScale ) / 4;
  }
  else
  {
    return ( mWinWidth - 160 * mScale ) / 2;
  }
}

int WinRenderer::SizeManager::instanceYOff( int instance ) const
{
  if ( mInstances == 2 && !mHorizontal )
  {
    return ( ( instance == 1 ? 3 : 1 ) * mWinHeight - 204 * mScale ) / 4;
  }
  else
  {
    return ( mWinHeight - 102 * mScale ) / 2;
  }
}

int WinRenderer::SizeManager::scale() const
{
  return mScale;
}

bool WinRenderer::SizeManager::horizontal() const
{
  return mHorizontal;
}

WinRenderer::SizeManager::operator bool() const
{
  return mWinWidth != 0 && mWinHeight != 0;
}

WinRenderer::Instance::Instance() : mActiveFrame{}, mFinishedFrames{}, mQueueMutex{}, mFrameTicks{ ~0ull }
{
  for ( uint32_t i = 0; i < 256; ++i )
  {
    mPalette[i] = DPixel{ Pixel{ 0, 0, 0, 255 }, Pixel{ 0, 0, 0, 255 } };
  }
}

#include "pch.hpp"
#include "WinRenderer.hpp"
#include "Manager.hpp"
#include "imgui.h"
#include "WinImgui.hpp"
#include "renderer.hxx"
#include "Log.hpp"
#include "IEncoder.hpp"

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

  RenderFrame() : id{ sId++ }, currentRow{}
  {
    std::fill( sizes.begin(), sizes.end(), 0 );
    newRow( 104 );
  }

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

WinRenderer::WinRenderer() : mRenderer{}, mLastRenderTimePoint{}
{
  LARGE_INTEGER l;
  QueryPerformanceCounter( &l );
  mLastRenderTimePoint = l.QuadPart;
}

WinRenderer::~WinRenderer()
{
}

void WinRenderer::setEncoder( std::shared_ptr<IEncoder> encoder )
{
  if ( mRenderer )
    mRenderer->setEncoder( std::move( encoder ) );
}

void WinRenderer::initialize( HWND hWnd, std::filesystem::path const& iniPath )
{
  mRenderer = std::make_shared<WinRenderer::DX11Renderer>( hWnd, iniPath );
}

std::shared_ptr<IVideoSink> WinRenderer::getVideoSink() const
{
  assert( mRenderer );
  return mRenderer->getVideoSink();
}

int64_t WinRenderer::render( Manager& config )
{
  LARGE_INTEGER l;
  QueryPerformanceCounter( &l );

  mRenderer->render( config );

  auto result = l.QuadPart - mLastRenderTimePoint;
  mLastRenderTimePoint = l.QuadPart;
  return result;
}

int WinRenderer::win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch ( msg )
  {
  case WM_SIZING:
    return mRenderer->sizing( *(RECT*)lParam );
  case WM_SIZE:
    break;
  default:
    return mRenderer->win32_WndProcHandler( hWnd, msg, wParam, lParam );
  }

  return 0;
}

void WinRenderer::setRotation( ImageCart::Rotation rotation )
{
  assert( mRenderer );
  mRenderer->setRotation( rotation );
}


WinRenderer::SizeManager::SizeManager() : mWinWidth{}, mWinHeight{}, mScale{ 1 }
{
}

WinRenderer::SizeManager::SizeManager( int windowWidth, int windowHeight, ImageCart::Rotation rotation ) : mWinWidth{ windowWidth }, mWinHeight{ windowHeight }, mScale{ 1 }, mRotation{ rotation }
{
  int sx, sy;
  switch ( mRotation )
  {
  case ImageCart::Rotation::LEFT:
  case ImageCart::Rotation::RIGHT:
    sx = (std::max)( 1, mWinWidth / 102 );
    sy = (std::max)( 1, mWinHeight / 160 );
    break;
  default:
    sx = (std::max)( 1, mWinWidth / 160 );
    sy = (std::max)( 1, mWinHeight / 102 );
    break;
  }

  mScale = (std::min)( sx, sy );
}

int WinRenderer::SizeManager::windowWidth() const
{
  return mWinWidth;
}

int WinRenderer::SizeManager::windowHeight() const
{
  return mWinHeight;
}

int WinRenderer::SizeManager::minWindowWidth() const
{
  return mRotation == ImageCart::Rotation::NORMAL ? 160 : 102;
}

int WinRenderer::SizeManager::minWindowHeight() const
{
  return mRotation == ImageCart::Rotation::NORMAL ? 102 : 160;
}

int WinRenderer::SizeManager::xOff() const
{
  switch ( mRotation )
  {
  case ImageCart::Rotation::LEFT:
    return ( mWinWidth + 102 * mScale ) / 2;
  case ImageCart::Rotation::RIGHT:
    return ( mWinWidth - 102 * mScale ) / 2;
  default:
    return ( mWinWidth - 160 * mScale ) / 2;
  }
}

int WinRenderer::SizeManager::yOff() const
{
  switch ( mRotation )
  {
  case ImageCart::Rotation::LEFT:
    return ( mWinHeight - 160 * mScale ) / 2;
  case ImageCart::Rotation::RIGHT:
    return ( mWinHeight + 160 * mScale ) / 2;
  default:
    return ( mWinHeight - 102 * mScale ) / 2;
  }
}

int WinRenderer::SizeManager::scale() const
{
  return mScale;
}

int WinRenderer::SizeManager::rotx1() const
{
  switch ( mRotation )
  {
  case ImageCart::Rotation::LEFT:
    return 0;  //mScale * cos( 90 )
  case ImageCart::Rotation::RIGHT:
    return 0;  //mScale * cos( -90 )
  default:
    return mScale;  //mScale * cos( 0 )
  }
}

int WinRenderer::SizeManager::rotx2() const
{
  switch ( mRotation )
  {
  case ImageCart::Rotation::LEFT:
    return -mScale;  //mScale * -sin( 90 )
  case ImageCart::Rotation::RIGHT:
    return mScale;  //mScale * -sin( -90 )
  default:
    return 0;  //mScale * -sin( 0 )
  }
}

int WinRenderer::SizeManager::roty1() const
{
  switch ( mRotation )
  {
  case ImageCart::Rotation::LEFT:
    return mScale;  //mScale * sin( 90 )
  case ImageCart::Rotation::RIGHT:
    return -mScale;  //mScale * sin( -90 )
  default:
    return 0;  //mScale * sin( 0 )
  }
}

int WinRenderer::SizeManager::roty2() const
{
  switch ( mRotation )
  {
  case ImageCart::Rotation::LEFT:
    return 0;  //mScale * cos( 90 )
  case ImageCart::Rotation::RIGHT:
    return 0;  //mScale * cos( -90 )
  default:
    return mScale;  //mScale * cos( 0 )
  }
}

ImageCart::Rotation WinRenderer::SizeManager::rotation() const
{
  return mRotation;
}

WinRenderer::SizeManager::operator bool() const
{
  return mWinWidth != 0 && mWinHeight != 0;
}

void WinRenderer::Instance::newFrame( uint64_t tick, uint8_t hbackup )
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

WinRenderer::Instance::Instance() : mActiveFrame{}, mFinishedFrames{}, mQueueMutex{}, mBeginTick{}, mLastTick{}, mFrameTicks{ ~0ull }
{
  for ( uint32_t i = 0; i < 256; ++i )
  {
    mPalette[i] = DPixel{ Pixel{ 0, 0, 0, 255 }, Pixel{ 0, 0, 0, 255 } };
  }
}

WinRenderer::BaseRenderer::BaseRenderer( HWND hWnd ) : mHWnd{ hWnd }, mInstance{ std::make_shared<Instance>() }, mSizeManager{}, mRotation{}
{
}

std::shared_ptr<IVideoSink> WinRenderer::BaseRenderer::getVideoSink() const
{
  return mInstance;
}

void WinRenderer::BaseRenderer::updateSourceTexture( std::shared_ptr<RenderFrame> frame, std::shared_ptr<MappedTexture> map )
{
  for ( int i = 0; i < (int)frame->rows.size(); ++i )
  {
    auto const& row = frame->rows[i];
    int size = frame->sizes[i];
    DPixel* dst = map->data + std::max( 0, ( i - 3 ) ) * map->stride;

    for ( int j = 0; j < size; ++j )
    {
      uint16_t v = row.lineBuffer[j];
      if ( std::bit_cast<int16_t>( v ) < 0 )
      {
        *dst++ = mInstance->mPalette[(uint8_t)v];
      }
      else
      {
        mInstance->updatePalette( v >> 8, (uint8_t)v );
      }
    }
  }
}

int WinRenderer::BaseRenderer::sizing( RECT& rect )
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

  return 1;
}

void WinRenderer::BaseRenderer::setRotation( ImageCart::Rotation rotation )
{
  mRotation = rotation;
}


WinRenderer::DX11Renderer::DX11Renderer( HWND hWnd, std::filesystem::path const& iniPath ) : BaseRenderer{ hWnd }, mRefreshRate {}, mEncoder{}, mVScale{ ~0u }
{
  typedef HRESULT( WINAPI* LPD3D11CREATEDEVICE )( IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT32, CONST D3D_FEATURE_LEVEL*, UINT, UINT32, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext** );
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
    & featureLevelsRequested, 1, D3D11_SDK_VERSION, mD3DDevice.ReleaseAndGetAddressOf(), &featureLevelsSupported, mImmediateContext.ReleaseAndGetAddressOf() );

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
  V_THROW( pDXGIAdapter->GetParent( __uuidof( IDXGIFactory ), (void**)pIDXGIFactory.ReleaseAndGetAddressOf() ) );

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

  updateVscale( 0 );

  mImgui.reset( new WinImgui{ mHWnd, mD3DDevice, mImmediateContext, iniPath } );
}

void WinRenderer::DX11Renderer::render( Manager& config )
{
  internalRender( config );
  mSwapChain->Present( 1, 0 );
}

void WinRenderer::DX11Renderer::internalRender( Manager& config )
{
  RECT r;
  if ( ::GetClientRect( mHWnd, &r ) == 0 )
    return;

  if ( mSizeManager.windowHeight() != r.bottom || ( mSizeManager.windowWidth() != r.right ) || mSizeManager.rotation() != mRotation )
  {
    mSizeManager = SizeManager{ r.right, r.bottom, mRotation };

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

  if ( mEncoder )
  {
    uint32_t encoderScale = mEncoder->width() / 160;
    if ( encoderScale != mVScale )
    {
      updateVscale( encoderScale );
    }
  }

  UINT v[4] = { 255, 255, 255, 255 };
  mImmediateContext->ClearUnorderedAccessViewUint( mBackBufferUAV.Get(), v );
  std::array<ID3D11UnorderedAccessView*, 4> uavs{ mBackBufferUAV.Get(), mPreStagingYUAV.Get(), mPreStagingUUAV.Get(), mPreStagingVUAV.Get() };
  mImmediateContext->CSSetUnorderedAccessViews( 0, 4, uavs.data(), nullptr );

  {
    if ( auto frame = mInstance->pullNextFrame() )
    {
      D3D11_MAPPED_SUBRESOURCE map;
      mImmediateContext->Map( mSource.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map );
      std::shared_ptr<MappedTexture> pMap{ new MappedTexture{ (DPixel*)map.pData, map.RowPitch / (uint32_t)sizeof( DPixel ) }, [this]( MappedTexture* p )
      {
        mImmediateContext->Unmap( mSource.Get(), 0 );
      } };

      updateSourceTexture( std::move( frame ), std::move( pMap ) );
    }

    CBPosSize cbPosSize{
      mSizeManager.rotx1(), mSizeManager.rotx2(),
      mSizeManager.roty1(), mSizeManager.roty2(),
      mSizeManager.xOff(), mSizeManager.yOff(),
      mSizeManager.scale(),
      mVScale };
    mImmediateContext->UpdateSubresource( mPosSizeCB.Get(), 0, NULL, &cbPosSize, 0, 0 );
    mImmediateContext->CSSetConstantBuffers( 0, 1, mPosSizeCB.GetAddressOf() );
    mImmediateContext->CSSetShaderResources( 0, 1, mSourceSRV.GetAddressOf() );
    mImmediateContext->CSSetShader( mRendererCS.Get(), nullptr, 0 );
    mImmediateContext->Dispatch( 5, 51, 1 );

    uavs[0] = nullptr;
    uavs[1] = nullptr;
    uavs[2] = nullptr;
    uavs[3] = nullptr;
    mImmediateContext->CSSetUnorderedAccessViews( 0, 4, uavs.data(), nullptr );
  }

  if ( mEncoder )
  {
    mImmediateContext->CopyResource( mStagingY.Get(), mPreStagingY.Get() );
    mImmediateContext->CopyResource( mStagingU.Get(), mPreStagingU.Get() );
    mImmediateContext->CopyResource( mStagingV.Get(), mPreStagingV.Get() );

    D3D11_MAPPED_SUBRESOURCE resY, resU, resV;
    mImmediateContext->Map( mStagingY.Get(), 0, D3D11_MAP_READ, 0, &resY );
    mImmediateContext->Map( mStagingU.Get(), 0, D3D11_MAP_READ, 0, &resU );
    mImmediateContext->Map( mStagingV.Get(), 0, D3D11_MAP_READ, 0, &resV );

    if ( !mEncoder->writeFrame( (uint8_t const*)resY.pData, resY.RowPitch, (uint8_t const*)resU.pData, resU.RowPitch, (uint8_t const*)resV.pData, resV.RowPitch ) )
    {
      mEncoder->startEncoding( mRefreshRate.numerator(), mRefreshRate.denominator() );
      mEncoder->writeFrame( (uint8_t const*)resY.pData, resY.RowPitch, (uint8_t const*)resU.pData, resU.RowPitch, (uint8_t const*)resV.pData, resV.RowPitch );
    }

    mImmediateContext->Unmap( mStagingY.Get(), 0 );
    mImmediateContext->Unmap( mStagingU.Get(), 0 );
    mImmediateContext->Unmap( mStagingV.Get(), 0 );
  }


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

    std::array<ID3D11RenderTargetView* const, 1> rtv{};
    mImmediateContext->OMSetRenderTargets( 1, rtv.data(), nullptr );
  }
}

void WinRenderer::DX11Renderer::setEncoder( std::shared_ptr<IEncoder> encoder )
{
  mEncoder = std::move( encoder );
}

int WinRenderer::DX11Renderer::win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  return mImgui->win32_WndProcHandler( hWnd, msg, wParam, lParam );
}

void WinRenderer::DX11Renderer::updateVscale( uint32_t vScale )
{
  mVScale = vScale;

  if ( mVScale == 0 )
    return;

  uint32_t width = 160 * std::max( 1u, mVScale );
  uint32_t height = 102 * std::max( 1u, mVScale );

  {
    D3D11_TEXTURE2D_DESC descsrc{ width, height, 1, 1, DXGI_FORMAT_R8_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_UNORDERED_ACCESS };
    V_THROW( mD3DDevice->CreateTexture2D( &descsrc, nullptr, mPreStagingY.ReleaseAndGetAddressOf() ) );
    V_THROW( mD3DDevice->CreateUnorderedAccessView( mPreStagingY.Get(), NULL, mPreStagingYUAV.ReleaseAndGetAddressOf() ) );
  }

  {
    D3D11_TEXTURE2D_DESC descsrc{ width, height, 1, 1, DXGI_FORMAT_R8_UNORM, { 1, 0 }, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ };
    V_THROW( mD3DDevice->CreateTexture2D( &descsrc, nullptr, mStagingY.ReleaseAndGetAddressOf() ) );
  }

  {
    D3D11_TEXTURE2D_DESC descsrc{ width / 2, height / 2, 1, 1, DXGI_FORMAT_R8_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_UNORDERED_ACCESS };
    V_THROW( mD3DDevice->CreateTexture2D( &descsrc, nullptr, mPreStagingU.ReleaseAndGetAddressOf() ) );
    V_THROW( mD3DDevice->CreateUnorderedAccessView( mPreStagingU.Get(), NULL, mPreStagingUUAV.ReleaseAndGetAddressOf() ) );
  }

  {
    D3D11_TEXTURE2D_DESC descsrc{ width / 2, height / 2, 1, 1, DXGI_FORMAT_R8_UNORM, { 1, 0 }, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ };
    V_THROW( mD3DDevice->CreateTexture2D( &descsrc, nullptr, mStagingU.ReleaseAndGetAddressOf() ) );
  }

  {
    D3D11_TEXTURE2D_DESC descsrc{ width / 2, height / 2, 1, 1, DXGI_FORMAT_R8_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_UNORDERED_ACCESS };
    V_THROW( mD3DDevice->CreateTexture2D( &descsrc, nullptr, mPreStagingV.ReleaseAndGetAddressOf() ) );
    V_THROW( mD3DDevice->CreateUnorderedAccessView( mPreStagingV.Get(), NULL, mPreStagingVUAV.ReleaseAndGetAddressOf() ) );
  }

  {
    D3D11_TEXTURE2D_DESC descsrc{ width / 2, height / 2, 1, 1, DXGI_FORMAT_R8_UNORM, { 1, 0 }, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ };
    V_THROW( mD3DDevice->CreateTexture2D( &descsrc, nullptr, mStagingV.ReleaseAndGetAddressOf() ) );
  }
}




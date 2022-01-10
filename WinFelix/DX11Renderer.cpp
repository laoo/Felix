#include "pch.hpp"
#include "DX11Renderer.hpp"
#include "Log.hpp"
#include "renderer.hxx"
#include "renderer2.hxx"
#include "rendererYUV.hxx"
#include "board.hxx"
#include "ScreenRenderingBuffer.hpp"
#include "WinImgui11.hpp"
#include "Manager.hpp"
#include "IEncoder.hpp"
#include "fonts.hpp"
#include "VideoSink.hpp"

#define V_THROW(x) { HRESULT hr_ = (x); if( FAILED( hr_ ) ) { throw std::runtime_error{ "DXError" }; } }

DX11Renderer::DX11Renderer( HWND hWnd, std::filesystem::path const& iniPath ) : BaseRenderer{ hWnd }, mBoardFont{}, mHexFont{}, mRefreshRate{}, mEncoder{}, mWindowRenderings{}, mVScale{ ~0u }
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
  V_THROW( mD3DDevice->CreateComputeShader( g_Renderer2, sizeof g_Renderer2, nullptr, mRenderer2CS.ReleaseAndGetAddressOf() ) );
  V_THROW( mD3DDevice->CreateComputeShader( g_RendererYUV, sizeof g_RendererYUV, nullptr, mRendererYUVCS.ReleaseAndGetAddressOf() ) );
  V_THROW( mD3DDevice->CreateComputeShader( g_Board, sizeof g_Board, nullptr, mBoardCS.ReleaseAndGetAddressOf() ) );

  D3D11_BUFFER_DESC bd = {};
  bd.ByteWidth = sizeof( CBPosSize );
  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  V_THROW( mD3DDevice->CreateBuffer( &bd, NULL, mPosSizeCB.ReleaseAndGetAddressOf() ) );

  bd.ByteWidth = sizeof( CBVSize );
  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  V_THROW( mD3DDevice->CreateBuffer( &bd, NULL, mVSizeCB.ReleaseAndGetAddressOf() ) );

  D3D11_TEXTURE2D_DESC desc{};
  desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
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

  std::vector<uint32_t> buf;
  buf.resize( desc.Width* desc.Height, ~0 );
  D3D11_SUBRESOURCE_DATA data{ buf.data(), desc.Width * sizeof( uint32_t ), 0 };
  mD3DDevice->CreateTexture2D( &desc, &data, mSource.ReleaseAndGetAddressOf() );
  V_THROW( mD3DDevice->CreateShaderResourceView( mSource.Get(), NULL, mSourceSRV.ReleaseAndGetAddressOf() ) );

  D3D11_TEXTURE2D_DESC descsrc{ 80, 102, 1, 1, DXGI_FORMAT_R8_UINT, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
  mD3DDevice->CreateTexture2D( &descsrc, nullptr, mWindowRenderings.source.ReleaseAndGetAddressOf() );
  V_THROW( mD3DDevice->CreateShaderResourceView( mWindowRenderings.source.Get(), NULL, mWindowRenderings.sourceSRV.ReleaseAndGetAddressOf() ) );

  D3D11_TEXTURE1D_DESC descpal{ 16, 1, 1, DXGI_FORMAT_B8G8R8A8_UNORM, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
  V_THROW( mD3DDevice->CreateTexture1D( &descpal, nullptr, mWindowRenderings.palette.ReleaseAndGetAddressOf() ) );
  V_THROW( mD3DDevice->CreateShaderResourceView( mWindowRenderings.palette.Get(), NULL, mWindowRenderings.paletteSRV.ReleaseAndGetAddressOf() ) );

  updateVscale( 0 );
  mBoardFont.initialize( mD3DDevice.Get(), mImmediateContext.Get() );
  mHexFont.initialize( mD3DDevice.Get(), mImmediateContext.Get() );

  mImgui = std::make_shared<WinImgui11>( mHWnd, mD3DDevice, mImmediateContext, iniPath );
}

void DX11Renderer::present()
{
  mSwapChain->Present( 1, 0 );
}

bool DX11Renderer::resizeOutput()
{
  RECT r;
  if ( ::GetClientRect( mHWnd, &r ) == 0 )
    return true;

  if ( mScreenGeometry.windowHeight() != r.bottom || ( mScreenGeometry.windowWidth() != r.right ) || mScreenGeometry.rotation() != mRotation )
  {
    mScreenGeometry = ScreenGeometry{ r.right, r.bottom, mRotation };

    if ( !mScreenGeometry )
    {
      return true;
    }

    mBackBufferUAV.Reset();
    mBackBufferRTV.Reset();

    mSwapChain->ResizeBuffers( 0, mScreenGeometry.windowWidth(), mScreenGeometry.windowHeight(), DXGI_FORMAT_UNKNOWN, 0 );

    ComPtr<ID3D11Texture2D> backBuffer;
    V_THROW( mSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)backBuffer.ReleaseAndGetAddressOf() ) );
    V_THROW( mD3DDevice->CreateUnorderedAccessView( backBuffer.Get(), nullptr, mBackBufferUAV.ReleaseAndGetAddressOf() ) );
    V_THROW( mD3DDevice->CreateRenderTargetView( backBuffer.Get(), nullptr, mBackBufferRTV.ReleaseAndGetAddressOf() ) );
  }
  return false;
}

void DX11Renderer::updateSourceFromNextFrame()
{
  if ( auto frame = mVideoSink->pullNextFrame() )
  {
    D3D11_MAPPED_SUBRESOURCE d3dmap;
    mImmediateContext->Map( mSource.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &d3dmap );

    struct MappedTexture
    {
      VideoSink::DPixel* data;
      uint32_t stride;
    } map{ (VideoSink::DPixel*)d3dmap.pData, d3dmap.RowPitch / (uint32_t)sizeof( VideoSink::DPixel ) };

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

    mImmediateContext->Unmap( mSource.Get(), 0 );
  }
}

void DX11Renderer::renderGui( UI& ui )
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

  ui.drawGui( r.left, r.top, r.right, r.bottom );

  ImGui::Render();

  mImmediateContext->OMSetRenderTargets( 1, mBackBufferRTV.GetAddressOf(), nullptr );
  mImgui->dx11_RenderDrawData( ImGui::GetDrawData() );

  std::array<ID3D11RenderTargetView* const, 1> rtv{};
  mImmediateContext->OMSetRenderTargets( 1, rtv.data(), nullptr );
}

void DX11Renderer::internalRender( UI& ui )
{
  if ( resizeOutput() )
    return;

  updateSourceFromNextFrame();

  UINT v[4] = { 255, 255, 255, 255 };
  mImmediateContext->ClearUnorderedAccessViewUint( mBackBufferUAV.Get(), v );

  if ( mWindowRenderings.main.enabled() )
  {
    mWindowRenderings.main.update( *this );
    mImmediateContext->ClearUnorderedAccessViewUint( mWindowRenderings.main.uav.Get(), v );
    renderScreenView( mWindowRenderings.main.geometry, mWindowRenderings.main.uav.Get() );
  }
  else
  {
    renderScreenView( mScreenGeometry, mBackBufferUAV.Get() );
  }

  if ( mEncoder )
  {
    renderEncoding();
  }

  renderGui( ui );
}

void DX11Renderer::renderScreenView( ScreenGeometry const& geometry, ID3D11UnorderedAccessView * target )
{
  mImmediateContext->CSSetUnorderedAccessViews( 0, 1, &target, nullptr );

  CBPosSize cbPosSize{
    geometry.rotx1(), geometry.rotx2(),
    geometry.roty1(), geometry.roty2(),
    geometry.xOff(), geometry.yOff(),
    geometry.scale()
  };

  mImmediateContext->UpdateSubresource( mPosSizeCB.Get(), 0, NULL, &cbPosSize, 0, 0 );
  mImmediateContext->CSSetConstantBuffers( 0, 1, mPosSizeCB.GetAddressOf() );
  mImmediateContext->CSSetShaderResources( 0, 1, mSourceSRV.GetAddressOf() );
  mImmediateContext->CSSetShader( mRendererCS.Get(), nullptr, 0 );
  mImmediateContext->Dispatch( 5, 51, 1 );

  target = nullptr;
  mImmediateContext->CSSetUnorderedAccessViews( 0, 1, &target, nullptr );
}

std::span<uint32_t const,16> DX11Renderer::safePalette()
{
  static constexpr std::array<uint32_t, 16> palette = {
    0xff000000,
    0xff0000aa,
    0xff00aa00,
    0xff00aaaa,
    0xffaa0000,
    0xffaa00aa,
    0xffaaaa00,
    0xffaaaaaa,
    0xff555555,
    0xff5555ff,
    0xff55ff55,
    0xff55ffff,
    0xffff5555,
    0xffff55ff,
    0xffffff55,
    0xffffffff
  };

  return std::span<uint32_t const, 16>( palette.data(), 16 );
}

void DX11Renderer::renderEncoding()
{
  uint32_t encoderScale = mEncoder->width() / 160;
  if ( encoderScale != mVScale )
  {
    updateVscale( encoderScale );
  }

  std::array<ID3D11UnorderedAccessView*, 3> uavs{ mPreStagingYUAV.Get(), mPreStagingUUAV.Get(), mPreStagingVUAV.Get() };
  mImmediateContext->CSSetUnorderedAccessViews( 0, 3, uavs.data(), nullptr );

  CBVSize cbVSize{ mVScale };

  mImmediateContext->UpdateSubresource( mVSizeCB.Get(), 0, NULL, &cbVSize, 0, 0 );
  mImmediateContext->CSSetConstantBuffers( 0, 1, mVSizeCB.GetAddressOf() );
  mImmediateContext->CSSetShaderResources( 0, 1, mSourceSRV.GetAddressOf() );
  mImmediateContext->CSSetShader( mRendererYUVCS.Get(), nullptr, 0 );
  mImmediateContext->Dispatch( 5, 51, 1 );


  uavs[0] = nullptr;
  uavs[1] = nullptr;
  uavs[2] = nullptr;
  mImmediateContext->CSSetUnorderedAccessViews( 0, 4, uavs.data(), nullptr );


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

void DX11Renderer::setEncoder( std::shared_ptr<IEncoder> encoder )
{
  mEncoder = std::move( encoder );
}

int DX11Renderer::win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
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

bool DX11Renderer::canRenderBoards() const
{
  return true;
}

ImTextureID DX11Renderer::renderBoard( int id, int width, int height, std::span<uint8_t const> data )
{
  auto it = mBoards.find( id );
  if ( it != mBoards.end() )
  {
    it->second.update( *this, width, height );
  }
  else
  {
    bool success;
    std::tie( it, success ) = mBoards.insert( { id, Board{} } );
    it->second.update( *this, width, height );
  }

  it->second.render( *this, data );
  return it->second.srv.Get();
}

ImTextureID DX11Renderer::mainRenderingTexture( int w, int h )
{
  mWindowRenderings.main.width = w;
  mWindowRenderings.main.height = h;
  return mWindowRenderings.main.srv.Get();
}

ImTextureID DX11Renderer::screenViewRenderingTexture( int id, ScreenViewType type, std::span<uint8_t const> data, std::span<uint8_t const> palette, int width, int height )
{
  auto it = mWindowRenderings.screenViews.find( id );
  if ( it != mWindowRenderings.screenViews.end() )
  {
    it->second.update( *this, width, height );
  }
  else
  {
    bool success;
    std::tie( it, success ) = mWindowRenderings.screenViews.insert( { id, DebugRendering{} } );
    it->second.update( *this, width, height );
  }

  it->second.render( *this, type, data, palette );
  return it->second.srv.Get();
}


void DX11Renderer::updateVscale( uint32_t vScale )
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


void DX11Renderer::Board::render( DX11Renderer& r, std::span<uint8_t const> data )
{
  r.mImmediateContext->UpdateSubresource( src.Get(), 0, NULL, data.data(), (uint32_t)width, 0 );
  std::array<ID3D11ShaderResourceView* const, 2> srv{ r.mBoardFont.srv.Get(), srcSRV.Get() };
  r.mImmediateContext->CSSetShaderResources( 0, 2, srv.data() );
  r.mImmediateContext->CSSetUnorderedAccessViews( 0, 1, uav.GetAddressOf(), nullptr );
  r.mImmediateContext->CSSetShader( r.mBoardCS.Get(), nullptr, 0 );

  r.mImmediateContext->Dispatch( width, height, 1 );

  std::array<ID3D11UnorderedAccessView* const, 1> uav{};
  r.mImmediateContext->CSSetUnorderedAccessViews( 0, 1, uav.data(), nullptr );

  std::array<ID3D11ShaderResourceView* const, 2> srvc{};
  r.mImmediateContext->CSSetShaderResources( 0, 2, srvc.data() );
}

DX11Renderer::BoardFont::BoardFont() : width{}, height{}, srv{}
{
}

void DX11Renderer::BoardFont::initialize( ID3D11Device* pDevice, ID3D11DeviceContext* pContext )
{
  width = 8;
  height = 16;

  std::array<D3D11_SUBRESOURCE_DATA, 256> initData;

  for ( size_t i = 0; i < initData.size(); ++i )
  {
    auto& ini = initData[i];
    ini.pSysMem = font_SWISSBX2 + i * height * width;
    ini.SysMemPitch = width;
    ini.SysMemSlicePitch = 0;
  }

  ComPtr<ID3D11Texture2D> tex;
  D3D11_TEXTURE2D_DESC descsrc{ (uint32_t)width, (uint32_t)height, 1, (uint32_t)initData.size(), DXGI_FORMAT_R8_UNORM, { 1, 0 }, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
  V_THROW( pDevice->CreateTexture2D( &descsrc, initData.data(), tex.ReleaseAndGetAddressOf() ) );
  V_THROW( pDevice->CreateShaderResourceView( tex.Get(), NULL, srv.ReleaseAndGetAddressOf() ) );
}

DX11Renderer::HexFont::HexFont() : srv{}
{
}

void DX11Renderer::HexFont::initialize( ID3D11Device* pDevice, ID3D11DeviceContext* pContext )
{
  std::array<D3D11_SUBRESOURCE_DATA, 256> initData;
  std::vector<uint8_t> buf;
  buf.resize( initData.size() * width * height );

  for ( size_t i = 0; i < initData.size(); ++i )
  {
    uint8_t * charBuf = &buf[i * width * height];
    size_t leftIdx = i >> 4;
    size_t rightIdx = i & 0x0f;

    for ( size_t y = 0; y < srcHeight; ++y )
    {
      uint8_t* rowBuf = &charBuf[( y + 2 ) * width];
      std::copy_n( src( leftIdx, y ), srcWidth, rowBuf + 1 );
      std::copy_n( src( rightIdx, y ), srcWidth, rowBuf + 8 );
    }

    auto& ini = initData[i];
    ini.pSysMem = charBuf;
    ini.SysMemPitch = width;
    ini.SysMemSlicePitch = 0;
  }

  ComPtr<ID3D11Texture2D> tex;
  D3D11_TEXTURE2D_DESC descsrc{ (uint32_t)width, (uint32_t)height, 1, (uint32_t)initData.size(), DXGI_FORMAT_R8_UNORM, { 1, 0 }, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
  V_THROW( pDevice->CreateTexture2D( &descsrc, initData.data(), tex.ReleaseAndGetAddressOf() ) );
  V_THROW( pDevice->CreateShaderResourceView( tex.Get(), NULL, srv.ReleaseAndGetAddressOf() ) );
}

uint8_t const* DX11Renderer::HexFont::src( size_t idx, size_t row )
{
  return &hex_6x12[idx * srcWidth * srcHeight + row * srcWidth];
}

bool DX11Renderer::DebugRendering::enabled() const
{
  return width != 0 && height != 0;
}

void DX11Renderer::DebugRendering::update( DX11Renderer& r, int w, int h )
{
  width = w;
  height = h;
  update( r );
}

void DX11Renderer::Board::update( DX11Renderer& r, int w, int h )
{
  if ( w == width && h == height )
    return;

  width = w;
  height = h;

  D3D11_TEXTURE2D_DESC desc{
    (uint32_t)( w * r.mBoardFont.width ),
    (uint32_t)( h * r.mBoardFont.height ),
    1,
    1,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    { 1, 0 },
    D3D11_USAGE_DEFAULT,
    D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,
    0,
    0
  };

  ComPtr<ID3D11Texture2D> tex;
  V_THROW( r.mD3DDevice->CreateTexture2D( &desc, nullptr, tex.ReleaseAndGetAddressOf() ) );
  V_THROW( r.mD3DDevice->CreateShaderResourceView( tex.Get(), NULL, srv.ReleaseAndGetAddressOf() ) );
  V_THROW( r.mD3DDevice->CreateUnorderedAccessView( tex.Get(), NULL, uav.ReleaseAndGetAddressOf() ) );

  D3D11_TEXTURE2D_DESC descsrc{ (uint32_t)w, (uint32_t)h, 1, 1, DXGI_FORMAT_R8_UINT, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
  V_THROW( r.mD3DDevice->CreateTexture2D( &descsrc, nullptr, src.ReleaseAndGetAddressOf() ) );
  V_THROW( r.mD3DDevice->CreateShaderResourceView( src.Get(), NULL, srcSRV.ReleaseAndGetAddressOf() ) );
}


void DX11Renderer::DebugRendering::update( DX11Renderer& r )
{
  if ( width == 0 || height == 0 )
  {
    srv.ReleaseAndGetAddressOf();
    uav.ReleaseAndGetAddressOf();
    geometry = ScreenGeometry{};
    return;
  }

  if ( geometry.windowHeight() != height || ( geometry.windowWidth() != width ) || geometry.rotation() != r.mRotation )
  {
    geometry = ScreenGeometry{ width, height, r.mRotation };

    if ( !geometry )
    {
      srv.ReleaseAndGetAddressOf();
      uav.ReleaseAndGetAddressOf();
      return;
    }

    D3D11_TEXTURE2D_DESC desc{
      (uint32_t)width,
      (uint32_t)height,
      1,
      1,
      DXGI_FORMAT_R8G8B8A8_UNORM,
      { 1, 0 },
      D3D11_USAGE_DEFAULT,
      D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,
      0,
      0
    };

    ComPtr<ID3D11Texture2D> tex;
    V_THROW( r.mD3DDevice->CreateTexture2D( &desc, nullptr, tex.ReleaseAndGetAddressOf() ) );
    V_THROW( r.mD3DDevice->CreateShaderResourceView( tex.Get(), NULL, srv.ReleaseAndGetAddressOf() ) );
    V_THROW( r.mD3DDevice->CreateUnorderedAccessView( tex.Get(), NULL, uav.ReleaseAndGetAddressOf() ) );
  }
}

void DX11Renderer::DebugRendering::render( DX11Renderer& r, ScreenViewType type, std::span<uint8_t const> data, std::span<uint8_t const> palette )
{
  if ( enabled() )
  {
    auto rawUAV = uav.Get();
    if ( !rawUAV )
      return;

    UINT v[4] = { 0, 0, 0, 0 };
    r.mImmediateContext->ClearUnorderedAccessViewUint( rawUAV, v );

    if ( data.empty() )
      return;

    if ( palette.size() != 32 )
    {
      auto pal = DX11Renderer::safePalette();
      r.mImmediateContext->UpdateSubresource( r.mWindowRenderings.palette.Get(), 0, nullptr, pal.data(), 16 * 4, 0 );
    }
    else
    {
      uint32_t pal[16];

      for ( size_t i = 0; i < 16; ++i )
      {
        VideoSink::Pixel value;
        value.x = 0xff;
        value.r = ( palette[i + 16] & 0x0f );
        value.r |= value.r << 4;
        value.g = ( palette[i] & 0x0f );
        value.g |= value.g << 4;
        value.b = ( palette[i + 16] & 0xf0 );
        value.b |= value.r >> 4;

        pal[i] = std::bit_cast<uint32_t>( value );
      }

      r.mImmediateContext->UpdateSubresource( r.mWindowRenderings.palette.Get(), 0, nullptr, pal, 16 * 4, 0 );
    }

    r.mImmediateContext->UpdateSubresource( r.mWindowRenderings.source.Get(), 0, nullptr, data.data(), 80, 0 );

    r.mImmediateContext->CSSetUnorderedAccessViews( 0, 1, &rawUAV, nullptr );

    CBPosSize cbPosSize{
      geometry.rotx1(), geometry.rotx2(),
      geometry.roty1(), geometry.roty2(),
      geometry.xOff(), geometry.yOff(),
      geometry.scale()
    };

    r.mImmediateContext->UpdateSubresource( r.mPosSizeCB.Get(), 0, NULL, &cbPosSize, 0, 0 );
    r.mImmediateContext->CSSetConstantBuffers( 0, 1, r.mPosSizeCB.GetAddressOf() );
    ID3D11ShaderResourceView* tab[] = { r.mWindowRenderings.sourceSRV.Get(), r.mWindowRenderings.paletteSRV.Get() };
    r.mImmediateContext->CSSetShaderResources( 0, 2, tab );
    r.mImmediateContext->CSSetShader( r.mRenderer2CS.Get(), nullptr, 0 );
    r.mImmediateContext->Dispatch( 5, 51, 1 );

    rawUAV = nullptr;
    r.mImmediateContext->CSSetUnorderedAccessViews( 0, 1, &rawUAV, nullptr );
  }
}

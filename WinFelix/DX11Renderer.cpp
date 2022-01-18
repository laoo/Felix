#include "pch.hpp"
#include "DX11Renderer.hpp"
#include "DX11Helpers.hpp"
#include "Log.hpp"
#include "renderer.hxx"
#include "renderer2.hxx"
#include "board.hxx"
#include "ScreenRenderingBuffer.hpp"
#include "WinImgui11.hpp"
#include "Manager.hpp"
#include "IEncoder.hpp"
#include "fonts.hpp"
#include "VideoSink.hpp"
#include "EncodingRenderer.hpp"

#define V_THROW(x) { HRESULT hr_ = (x); if( FAILED( hr_ ) ) { throw std::runtime_error{ "DXError" }; } }
#define V_RETURN_FALSE(x) { HRESULT hr_ = (x); if( FAILED( hr_ ) ) { return false; } }

namespace
{

struct CBPosSize
{
  int32_t posx;
  int32_t posy;
  int32_t rotx1;
  int32_t rotx2;
  int32_t roty1;
  int32_t roty2;
  int32_t size;
  uint32_t padding;
};

static ComPtr<ID3D11Device>        gD3DDevice;
static ComPtr<ID3D11DeviceContext> gImmediateContext;
static ComPtr<ID3D11ComputeShader> gRenderer2CS;
static ComPtr<ID3D11ComputeShader> gRendererCS;
static ComPtr<ID3D11ComputeShader> gBoardCS;

}

DX11Renderer::DX11Renderer( HWND hWnd, std::filesystem::path const& iniPath ) : BaseRenderer{ hWnd }, mBoardFont{}, mHexFont{}, mRefreshRate{}, mEncodingRenderer{}
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
    & featureLevelsRequested, 1, D3D11_SDK_VERSION, gD3DDevice.ReleaseAndGetAddressOf(), &featureLevelsSupported, gImmediateContext.ReleaseAndGetAddressOf() );

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
  V_THROW( gD3DDevice.As( &pDXGIDevice ) );

  ComPtr<IDXGIAdapter> pDXGIAdapter;
  V_THROW( pDXGIDevice->GetAdapter( pDXGIAdapter.ReleaseAndGetAddressOf() ) );

  ComPtr<IDXGIFactory> pIDXGIFactory;
  V_THROW( pDXGIAdapter->GetParent( __uuidof( IDXGIFactory ), (void**)pIDXGIFactory.ReleaseAndGetAddressOf() ) );

  V_THROW( pIDXGIFactory->CreateSwapChain( gD3DDevice.Get(), &sd, mSwapChain.ReleaseAndGetAddressOf() ) );

  ComPtr<IDXGIOutput> pIDXGIOutput;
  V_THROW( mSwapChain->GetContainingOutput( pIDXGIOutput.ReleaseAndGetAddressOf() ) );

  DXGI_MODE_DESC md;
  V_THROW( pIDXGIOutput->FindClosestMatchingMode( &sd.BufferDesc, &md, gD3DDevice.Get() ) );
  mRefreshRate = { md.RefreshRate.Numerator, md.RefreshRate.Denominator };

  L_INFO << "Refresh Rate: " << mRefreshRate << " = " << ( (double)mRefreshRate.numerator() / (double)mRefreshRate.denominator() );

  V_THROW( gD3DDevice->CreateComputeShader( g_Renderer, sizeof g_Renderer, nullptr, gRendererCS.ReleaseAndGetAddressOf() ) );
  V_THROW( gD3DDevice->CreateComputeShader( g_Renderer2, sizeof g_Renderer2, nullptr, gRenderer2CS.ReleaseAndGetAddressOf() ) );
  V_THROW( gD3DDevice->CreateComputeShader( g_Board, sizeof g_Board, nullptr, gBoardCS.ReleaseAndGetAddressOf() ) );

  D3D11_BUFFER_DESC bd = {};
  bd.ByteWidth = sizeof( CBPosSize );
  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  V_THROW( gD3DDevice->CreateBuffer( &bd, NULL, mPosSizeCB.ReleaseAndGetAddressOf() ) );

  D3D11_TEXTURE2D_DESC desc{};
  desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  desc.Width = SCREEN_WIDTH;
  desc.Height = SCREEN_HEIGHT;
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
  gD3DDevice->CreateTexture2D( &desc, &data, mSource.ReleaseAndGetAddressOf() );
  V_THROW( gD3DDevice->CreateShaderResourceView( mSource.Get(), NULL, mSourceSRV.ReleaseAndGetAddressOf() ) );

  mBoardFont.initialize();
  mHexFont.initialize();

  mImgui = std::make_shared<WinImgui11>( mHWnd, gD3DDevice, gImmediateContext, iniPath );
}

DX11Renderer::~DX11Renderer()
{
}

void DX11Renderer::setEncoder( std::shared_ptr<IEncoder> encoder )
{
  mEncodingRenderer = std::make_shared<EncodingRenderer>( std::move( encoder ), gD3DDevice, gImmediateContext, mRefreshRate );
}

std::shared_ptr<IExtendedRenderer> DX11Renderer::extendedRenderer()
{
  return std::dynamic_pointer_cast<IExtendedRenderer>( shared_from_this() );
}

bool DX11Renderer::mainScreenViewDebugRendering( std::shared_ptr<ScreenView> mainScreenView )
{
  if ( mainScreenView )
  {
    if ( auto uav = mainScreenView->getUAV() )
    {
      UINT v[4] = { 255, 255, 255, 255 };
      gImmediateContext->ClearUnorderedAccessViewUint( uav, v );
      renderScreenView( mainScreenView->getGeometry(), mSourceSRV.Get(), uav );
      return true;
    }
  }

  return false;
}

void DX11Renderer::internalRender( UI& ui )
{
  if ( !resizeOutput() )
    return;

  updateSourceFromNextFrame();

  UINT v[4] = { 255, 255, 255, 255 };
  gImmediateContext->ClearUnorderedAccessViewUint( mBackBufferUAV.Get(), v );

  if ( !mainScreenViewDebugRendering( mMainScreenView.lock() ) )
  {
    renderScreenView( mScreenGeometry, mSourceSRV.Get(), mBackBufferUAV.Get() );
  }

  if ( mEncodingRenderer )
  {
    mEncodingRenderer->renderEncoding( mSourceSRV.Get() );
  }

  renderGui( ui );
}

void DX11Renderer::present()
{
  mSwapChain->Present( 1, 0 );
}

void DX11Renderer::updateRotation()
{
  if ( auto mainScreenView = mMainScreenView.lock() )
  {
    mainScreenView->update( mRotation );
  }
}

bool DX11Renderer::resizeOutput()
{
  RECT r;
  if ( ::GetClientRect( mHWnd, &r ) == 0 )
    return true;

  if ( mScreenGeometry.update( r.right, r.bottom, mRotation ) )
  {

    mBackBufferUAV.Reset();
    mBackBufferRTV.Reset();

    mSwapChain->ResizeBuffers( 0, mScreenGeometry.windowWidth(), mScreenGeometry.windowHeight(), DXGI_FORMAT_UNKNOWN, 0 );

    ComPtr<ID3D11Texture2D> backBuffer;
    V_RETURN_FALSE( mSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)backBuffer.ReleaseAndGetAddressOf() ) );
    V_RETURN_FALSE( gD3DDevice->CreateUnorderedAccessView( backBuffer.Get(), nullptr, mBackBufferUAV.ReleaseAndGetAddressOf() ) );
    V_RETURN_FALSE( gD3DDevice->CreateRenderTargetView( backBuffer.Get(), nullptr, mBackBufferRTV.ReleaseAndGetAddressOf() ) );
  }

  return (bool)mScreenGeometry;
}

void DX11Renderer::updateSourceFromNextFrame()
{
  if ( auto frame = mVideoSink->pullNextFrame() )
  {
    D3D11_MAPPED_SUBRESOURCE d3dmap;
    gImmediateContext->Map( mSource.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &d3dmap );

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

    gImmediateContext->Unmap( mSource.Get(), 0 );
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

  mImgui->newFrame();

  ImGui::NewFrame();

  ui.drawGui( r.left, r.top, r.right, r.bottom );

  ImGui::Render();

  RTVGuard rt{ gImmediateContext, mBackBufferRTV.Get() };
  mImgui->renderDrawData( ImGui::GetDrawData() );
}


void DX11Renderer::renderScreenView( ScreenGeometry const& geometry, ID3D11ShaderResourceView* sourceSRV, ID3D11UnorderedAccessView* target )
{
  UAVGuard uav{ gImmediateContext, target };

  CBPosSize cbPosSize{
    geometry.rotx1(), geometry.rotx2(),
    geometry.roty1(), geometry.roty2(),
    geometry.xOff(), geometry.yOff(),
    geometry.scale()
  };

  gImmediateContext->UpdateSubresource( mPosSizeCB.Get(), 0, NULL, &cbPosSize, 0, 0 );
  gImmediateContext->CSSetConstantBuffers( 0, 1, mPosSizeCB.GetAddressOf() );
  SRVGuard srvg{ gImmediateContext, sourceSRV };
  gImmediateContext->CSSetShader( gRendererCS.Get(), nullptr, 0 );
  gImmediateContext->Dispatch( SCREEN_WIDTH / 32, SCREEN_HEIGHT / 2, 1 );
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

std::shared_ptr<IScreenView> DX11Renderer::makeMainScreenView()
{
  if ( auto view = mMainScreenView.lock() )
  {
    return view;
  }
  else
  {
    auto ptr = std::make_shared<ScreenView>();
    ptr->update( mRotation );
    mMainScreenView = ptr;
    return ptr;
  }
}

std::shared_ptr<ICustomScreenView> DX11Renderer::makeCustomScreenView()
{
  return std::make_shared<CustomScreenView>();
}

std::span<uint32_t const, 16> DX11Renderer::safePalette()
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


void DX11Renderer::Board::render( DX11Renderer& r, std::span<uint8_t const> data )
{
  gImmediateContext->UpdateSubresource( src.Get(), 0, NULL, data.data(), (uint32_t)width, 0 );
  std::array<ID3D11ShaderResourceView* const, 2> srv{ r.mBoardFont.srv.Get(), srcSRV.Get() };
  gImmediateContext->CSSetShader( gBoardCS.Get(), nullptr, 0 );
  SRVGuard sg{ gImmediateContext, { r.mBoardFont.srv.Get(), srcSRV.Get() } };
  UAVGuard ug{ gImmediateContext, uav.Get() };
  gImmediateContext->Dispatch( width, height, 1 );
}

DX11Renderer::BoardFont::BoardFont() : width{}, height{}, srv{}
{
}

void DX11Renderer::BoardFont::initialize()
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
  V_THROW( gD3DDevice->CreateTexture2D( &descsrc, initData.data(), tex.ReleaseAndGetAddressOf() ) );
  V_THROW( gD3DDevice->CreateShaderResourceView( tex.Get(), NULL, srv.ReleaseAndGetAddressOf() ) );
}

DX11Renderer::HexFont::HexFont() : srv{}
{
}

void DX11Renderer::HexFont::initialize()
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
  V_THROW( gD3DDevice->CreateTexture2D( &descsrc, initData.data(), tex.ReleaseAndGetAddressOf() ) );
  V_THROW( gD3DDevice->CreateShaderResourceView( tex.Get(), NULL, srv.ReleaseAndGetAddressOf() ) );
}

uint8_t const* DX11Renderer::HexFont::src( size_t idx, size_t row )
{
  return &hex_6x12[idx * srcWidth * srcHeight + row * srcWidth];
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
  V_THROW( gD3DDevice->CreateTexture2D( &desc, nullptr, tex.ReleaseAndGetAddressOf() ) );
  V_THROW( gD3DDevice->CreateShaderResourceView( tex.Get(), NULL, srv.ReleaseAndGetAddressOf() ) );
  V_THROW( gD3DDevice->CreateUnorderedAccessView( tex.Get(), NULL, uav.ReleaseAndGetAddressOf() ) );

  D3D11_TEXTURE2D_DESC descsrc{ (uint32_t)w, (uint32_t)h, 1, 1, DXGI_FORMAT_R8_UINT, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
  V_THROW( gD3DDevice->CreateTexture2D( &descsrc, nullptr, src.ReleaseAndGetAddressOf() ) );
  V_THROW( gD3DDevice->CreateShaderResourceView( src.Get(), NULL, srcSRV.ReleaseAndGetAddressOf() ) );
}

DX11Renderer::ScreenView::ScreenView()
{
}

void DX11Renderer::ScreenView::update( ImageProperties::Rotation rotation )
{
  mGeometryChanged |= mGeometry.update( mGeometry.windowWidth(), mGeometry.windowHeight(), rotation );
}

void DX11Renderer::ScreenView::update( int width, int height )
{
  mGeometryChanged |= mGeometry.update( width, height, mGeometry.rotation() );
}

void* DX11Renderer::ScreenView::getTexture()
{
  return mSrv.Get();
}

ScreenGeometry const& DX11Renderer::ScreenView::getGeometry() const
{
  return mGeometry;
}

ID3D11UnorderedAccessView* DX11Renderer::ScreenView::getUAV()
{
  if ( mGeometryChanged )
    updateBuffers();

  return mUav.Get();
}

void DX11Renderer::ScreenView::updateBuffers()
{
  assert( mGeometryChanged );

  D3D11_TEXTURE2D_DESC desc{
    (uint32_t)( mGeometry.windowWidth() ),
    (uint32_t)( mGeometry.windowHeight() ),
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
  V_THROW( gD3DDevice->CreateTexture2D( &desc, nullptr, tex.ReleaseAndGetAddressOf() ) );
  V_THROW( gD3DDevice->CreateShaderResourceView( tex.Get(), NULL, mSrv.ReleaseAndGetAddressOf() ) );
  V_THROW( gD3DDevice->CreateUnorderedAccessView( tex.Get(), NULL, mUav.ReleaseAndGetAddressOf() ) );

  mGeometryChanged = false;
}

bool DX11Renderer::ScreenView::geometryChanged() const
{
  return mGeometryChanged;
}

DX11Renderer::CustomScreenView::CustomScreenView() : ScreenView{}
{
  D3D11_TEXTURE2D_DESC descsrc{ SCREEN_WIDTH / 2, SCREEN_HEIGHT, 1, 1, DXGI_FORMAT_R8_UINT, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
  gD3DDevice->CreateTexture2D( &descsrc, nullptr, mSource.ReleaseAndGetAddressOf() );
  V_THROW( gD3DDevice->CreateShaderResourceView( mSource.Get(), NULL, mSourceSRV.ReleaseAndGetAddressOf() ) );

  D3D11_TEXTURE1D_DESC descpal{ 16, 1, 1, DXGI_FORMAT_B8G8R8A8_UNORM, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
  V_THROW( gD3DDevice->CreateTexture1D( &descpal, nullptr, mPalette.ReleaseAndGetAddressOf() ) );
  V_THROW( gD3DDevice->CreateShaderResourceView( mPalette.Get(), NULL, mPaletteSRV.ReleaseAndGetAddressOf() ) );

  D3D11_BUFFER_DESC bd = {};
  bd.ByteWidth = sizeof( CBPosSize );
  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  V_THROW( gD3DDevice->CreateBuffer( &bd, NULL, mPosSizeCB.ReleaseAndGetAddressOf() ) );
}

void DX11Renderer::CustomScreenView::update( int width, int height )
{
  ScreenView::update( width, height );
}

void* DX11Renderer::CustomScreenView::update( std::span<uint8_t const> data, std::span<uint8_t const> palette )
{
  auto rawUAV = getUAV();
  if ( !rawUAV )
    return nullptr;

  UINT v[4] = { 255, 255, 255, 255 };
  gImmediateContext->ClearUnorderedAccessViewUint( rawUAV, v );

  if ( data.empty() )
    return nullptr;

  if ( palette.size() != 32 )
  {
    auto pal = DX11Renderer::safePalette();
    gImmediateContext->UpdateSubresource( mPalette.Get(), 0, nullptr, pal.data(), 16 * 4, 0 );
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

    gImmediateContext->UpdateSubresource( mPalette.Get(), 0, nullptr, pal, 16 * 4, 0 );
  }

  gImmediateContext->UpdateSubresource( mSource.Get(), 0, nullptr, data.data(), 80, 0 );

  CBPosSize cbPosSize{
    mGeometry.rotx1(), mGeometry.rotx2(),
    mGeometry.roty1(), mGeometry.roty2(),
    mGeometry.xOff(), mGeometry.yOff(),
    mGeometry.scale()
  };

  gImmediateContext->UpdateSubresource( mPosSizeCB.Get(), 0, NULL, &cbPosSize, 0, 0 );
  gImmediateContext->CSSetConstantBuffers( 0, 1, mPosSizeCB.GetAddressOf() );
  gImmediateContext->CSSetShader( gRenderer2CS.Get(), nullptr, 0 );
  UAVGuard ug{ gImmediateContext, rawUAV };
  SRVGuard sg{ gImmediateContext, { mSourceSRV.Get(), mPaletteSRV.Get() } };
  gImmediateContext->Dispatch( SCREEN_WIDTH / 32, SCREEN_HEIGHT / 2, 1 );

  return mSrv.Get();
}

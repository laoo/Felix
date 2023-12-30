#include "DX11Renderer.hpp"
#include "DX11Helpers.hpp"
#include "Log.hpp"
#include "renderer.hxx"
#include "renderer2.hxx"
#include "ScreenRenderingBuffer.hpp"
#include "WinImgui11.hpp"
#include "Manager.hpp"
#include "IEncoder.hpp"
#include "fonts.hpp"
#include "VideoSink.hpp"

#define V_THROW(x) { HRESULT hr_ = (x); if( FAILED( hr_ ) ) { throw std::runtime_error{ "DXError" }; } }
#define V_RETURN_FALSE(x) { HRESULT hr_ = (x); if( FAILED( hr_ ) ) { return false; } }

namespace
{

//CGA palette
static constexpr std::array<uint32_t, 16> gSafePalette = {
  0xff000000,
  0xff0000aa,
  0xff00aa00,
  0xff00aaaa,
  0xffaa0000,
  0xffaa00aa,
  0xffaa5500,
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

DX11Renderer::BoardFont gBoardFont;

static ComPtr<ID3D11Device>        gD3DDevice;
static ComPtr<ID3D11DeviceContext> gImmediateContext;
static ComPtr<ID3D11ComputeShader> gRenderer2CS;
static ComPtr<ID3D11ComputeShader> gRendererCS;

}

DX11Renderer::DX11Renderer( HWND hWnd, std::filesystem::path const& iniPath, Tag ) : mHWnd{ hWnd }, mRefreshRate{}, mVideoSink{ std::make_shared<VideoSink>() }, mLastRenderTimePoint{}
{
  LARGE_INTEGER l;
  QueryPerformanceCounter( &l );
  mLastRenderTimePoint = l.QuadPart;

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
  mRefreshRate = rational::Ratio<int32_t>{ (int)md.RefreshRate.Numerator, (int)md.RefreshRate.Denominator };

  L_INFO << "Refresh Rate: " << mRefreshRate.numer << '/' << mRefreshRate.denom << " = " << (float)mRefreshRate;

  V_THROW( gD3DDevice->CreateComputeShader( g_Renderer, sizeof g_Renderer, nullptr, gRendererCS.ReleaseAndGetAddressOf() ) );
  V_THROW( gD3DDevice->CreateComputeShader( g_Renderer2, sizeof g_Renderer2, nullptr, gRenderer2CS.ReleaseAndGetAddressOf() ) );

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

  gBoardFont.initialize();

  mImgui = std::make_shared<WinImgui11>( mHWnd, gD3DDevice, gImmediateContext, iniPath );
}

std::pair<std::shared_ptr<IBaseRenderer>, std::shared_ptr<IExtendedRenderer>> DX11Renderer::create( HWND hWnd, std::filesystem::path const& iniPath )
{
  std::shared_ptr<DX11Renderer> renderer = std::make_shared<DX11Renderer>( hWnd, iniPath, Tag{} );

  return { std::dynamic_pointer_cast<IBaseRenderer>( renderer ), std::dynamic_pointer_cast<IExtendedRenderer>( renderer ) };
}

DX11Renderer::~DX11Renderer()
{
}

int64_t DX11Renderer::render( UI& ui )
{
  LARGE_INTEGER l;
  QueryPerformanceCounter( &l );

  internalRender( ui );
  mSwapChain->Present( 1, 0 );

  auto result = l.QuadPart - mLastRenderTimePoint;
  mLastRenderTimePoint = l.QuadPart;
  return result;
}

void DX11Renderer::setRotation( ImageProperties::Rotation rotation )
{
  mRotation = rotation;
  if ( auto mainScreenView = mMainScreenView.lock() )
  {
    mainScreenView->rotate( mRotation );
  }
}

std::shared_ptr<IVideoSink> DX11Renderer::getVideoSink()
{
  return mVideoSink;
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

int DX11Renderer::sizing( RECT& rect )
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

  renderGui( ui );
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

std::shared_ptr<IScreenView> DX11Renderer::makeMainScreenView()
{
  if ( auto view = mMainScreenView.lock() )
  {
    return view;
  }
  else
  {
    auto ptr = std::make_shared<ScreenView>();
    ptr->rotate( mRotation );
    mMainScreenView = ptr;
    return ptr;
  }
}

std::shared_ptr<ICustomScreenView> DX11Renderer::makeCustomScreenView()
{
  return std::make_shared<CustomScreenView>();
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

DX11Renderer::ScreenView::ScreenView()
{
}

void DX11Renderer::ScreenView::rotate( ImageProperties::Rotation rotation )
{
  mGeometryChanged |= mGeometry.update( mGeometry.windowWidth(), mGeometry.windowHeight(), rotation );
}

void DX11Renderer::ScreenView::resize( int width, int height )
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

void DX11Renderer::CustomScreenView::resize( int width, int height )
{
  ScreenView::resize( width, height );
}

void* DX11Renderer::CustomScreenView::render( std::span<uint8_t const> data, std::span<uint8_t const> palette )
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
    gImmediateContext->UpdateSubresource( mPalette.Get(), 0, nullptr, gSafePalette.data(), 16 * 4, 0 );
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

int DX11Renderer::wndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
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

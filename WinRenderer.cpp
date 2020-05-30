#include "WinRenderer.hpp"
#include "renderer.hxx"

#define V_THROW(x) { HRESULT hr_ = (x); if( FAILED( hr_ ) ) { throw std::runtime_error{ "DXError" }; } }


WinRenderer::WinRenderer( HWND hWnd ) : mHWnd{ hWnd }, theWinWidth{}, theWinHeight{}
{
  typedef HRESULT( WINAPI * LPD3D11CREATEDEVICE )( IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT32, CONST D3D_FEATURE_LEVEL*, UINT, UINT32, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext** );
  static LPD3D11CREATEDEVICE  s_DynamicD3D11CreateDevice = nullptr;
  HMODULE hModD3D11 = ::LoadLibrary( L"d3d11.dll" );
  if ( hModD3D11 == nullptr )
    throw std::runtime_error{ "DXError" };

  s_DynamicD3D11CreateDevice = (LPD3D11CREATEDEVICE)GetProcAddress( hModD3D11, "D3D11CreateDevice" );


  D3D_FEATURE_LEVEL  featureLevelsRequested = D3D_FEATURE_LEVEL_11_0;
  UINT               numFeatureLevelsRequested = 1;
  D3D_FEATURE_LEVEL  featureLevelsSupported;

  HRESULT hr = s_DynamicD3D11CreateDevice( nullptr,  D3D_DRIVER_TYPE_HARDWARE, nullptr,
#ifndef NDEBUG
    D3D11_CREATE_DEVICE_DEBUG,
#else
    0,
#endif
    &featureLevelsRequested, numFeatureLevelsRequested, D3D11_SDK_VERSION, &mD3DDevice, &featureLevelsSupported, &mImmediateContext );

  V_THROW( hr );

  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory( &sd, sizeof( sd ) );
  sd.BufferCount = 2;
  sd.BufferDesc.Width = 0;
  sd.BufferDesc.Height = 0;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.RefreshRate.Numerator = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.BufferUsage = DXGI_USAGE_UNORDERED_ACCESS;
  sd.OutputWindow = mHWnd;
  sd.SampleDesc.Count = 1;
  sd.SampleDesc.Quality = 0;
  sd.Windowed = TRUE;

  ATL::CComPtr<IDXGIDevice> pDXGIDevice;
  V_THROW( mD3DDevice->QueryInterface( __uuidof( IDXGIDevice ), (void **)&pDXGIDevice ) );

  ATL::CComPtr<IDXGIAdapter> pDXGIAdapter;
  V_THROW( pDXGIDevice->GetParent( __uuidof( IDXGIAdapter ), (void **)&pDXGIAdapter ) );

  ATL::CComPtr<IDXGIFactory> pIDXGIFactory;
  V_THROW( pDXGIAdapter->GetParent( __uuidof( IDXGIFactory ), (void **)&pIDXGIFactory ) );

  ATL::CComPtr<IDXGISwapChain> pSwapChain;
  V_THROW( pIDXGIFactory->CreateSwapChain( mD3DDevice, &sd, &pSwapChain ) );

  mSwapChain = std::move( pSwapChain );

  V_THROW( mD3DDevice->CreateComputeShader( g_Renderer, sizeof g_Renderer, nullptr, &mRendererCS ) );

  D3D11_BUFFER_DESC bd ={};
  bd.ByteWidth = sizeof( CBPosSize );
  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  V_THROW( mD3DDevice->CreateBuffer( &bd, NULL, &mPosSizeCB ) );

}

void WinRenderer::render( DisplayGenerator::Pixel const * surface )
{
  D3D11_TEXTURE2D_DESC desc{};
  desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.Width = 160;
  desc.Height = 102;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Usage = D3D11_USAGE_IMMUTABLE;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = 0;
  desc.MiscFlags = 0;
  D3D11_SUBRESOURCE_DATA sda{ surface, 160 * 4, 0 };
  ATL::CComPtr<ID3D11Texture2D> tex;
  mD3DDevice->CreateTexture2D( &desc, &sda, &tex );
  ATL::CComPtr<ID3D11ShaderResourceView> texSRV;
  V_THROW( mD3DDevice->CreateShaderResourceView( tex, NULL, &texSRV.p ) );

  
  RECT r;
  ::GetClientRect( mHWnd, &r );

  if ( theWinHeight != ( r.bottom - r.top ) || ( theWinWidth != r.right - r.left ) )
  {
    theWinHeight = r.bottom - r.top;
    theWinWidth = r.right - r.left;

    if ( theWinHeight == 0 || theWinWidth == 0 )
    {
      return;
    }

    mBackBuffer.Release();
    mBackBufferUAV.Release();

    mSwapChain->ResizeBuffers( 0, theWinWidth, theWinHeight, DXGI_FORMAT_UNKNOWN, 0 );

    V_THROW( mSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)&mBackBuffer ) );
    V_THROW( mD3DDevice->CreateUnorderedAccessView( mBackBuffer, nullptr, &mBackBufferUAV ) );

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
  mImmediateContext->UpdateSubresource( mPosSizeCB, 0, NULL, &cbPosSize, 0, 0 );
  mImmediateContext->CSSetConstantBuffers( 0, 1, &mPosSizeCB.p );
  mImmediateContext->CSSetShaderResources( 0, 1, &texSRV.p );
  mImmediateContext->CSSetUnorderedAccessViews( 0, 1, &mBackBufferUAV.p, nullptr );
  mImmediateContext->CSSetShader( mRendererCS, nullptr, 0 );
  UINT v[4]={};
  mImmediateContext->ClearUnorderedAccessViewUint( mBackBufferUAV, v );
  mImmediateContext->Dispatch( 10, 102, 1 );
  mSwapChain->Present( 1, 0 );
}


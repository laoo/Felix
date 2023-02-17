#include "pch.hpp"
#include "EncodingRenderer.hpp"
#include "IEncoder.hpp"
#include "rendererYUV.hxx"
#include "Utility.hpp"
#include "DX11Helpers.hpp"

#define V_THROW(x) { HRESULT hr_ = (x); if( FAILED( hr_ ) ) { throw std::runtime_error{ "DXError" }; } }

EncodingRenderer::EncodingRenderer( std::shared_ptr<IEncoder> encoder, ComPtr<ID3D11Device> pD3DDevice, ComPtr<ID3D11DeviceContext> pImmediateContext, rational::Ratio<int32_t> refreshRate ) :
  mEncoder{ std::move( encoder ) }, mD3DDevice{ std::move( pD3DDevice ) }, mImmediateContext{ std::move( pImmediateContext ) }, mCb{}, mRefreshRate{ refreshRate }
{
  mCb.vscale = mEncoder->width() / SCREEN_WIDTH;

  if ( mCb.vscale == 0 )
    return;
  uint32_t width = SCREEN_WIDTH * std::max( 1u, mCb.vscale );
  uint32_t height = SCREEN_HEIGHT * std::max( 1u, mCb.vscale );

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

  V_THROW( mD3DDevice->CreateComputeShader( g_RendererYUV, sizeof g_RendererYUV, nullptr, mRendererYUVCS.ReleaseAndGetAddressOf() ) );

  D3D11_BUFFER_DESC bd{ sizeof( CBVSize ), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
  V_THROW( mD3DDevice->CreateBuffer( &bd, NULL, mVSizeCB.ReleaseAndGetAddressOf() ) );
}

void EncodingRenderer::renderEncoding( ID3D11ShaderResourceView* srv )
{
  {
    UAVGuard ug{ mImmediateContext, { mPreStagingYUAV.Get(), mPreStagingUUAV.Get(), mPreStagingVUAV.Get() } };
    SRVGuard sg{ mImmediateContext, srv };

    mImmediateContext->UpdateSubresource( mVSizeCB.Get(), 0, NULL, &mCb, 0, 0 );
    mImmediateContext->CSSetConstantBuffers( 0, 1, mVSizeCB.GetAddressOf() );
    mImmediateContext->CSSetShader( mRendererYUVCS.Get(), nullptr, 0 );

    mImmediateContext->Dispatch( SCREEN_WIDTH / 32, SCREEN_HEIGHT / 2, 1 );
  }

  mImmediateContext->CopyResource( mStagingY.Get(), mPreStagingY.Get() );
  mImmediateContext->CopyResource( mStagingU.Get(), mPreStagingU.Get() );
  mImmediateContext->CopyResource( mStagingV.Get(), mPreStagingV.Get() );

  D3D11_MAPPED_SUBRESOURCE resY, resU, resV;
  mImmediateContext->Map( mStagingY.Get(), 0, D3D11_MAP_READ, 0, &resY );
  mImmediateContext->Map( mStagingU.Get(), 0, D3D11_MAP_READ, 0, &resU );
  mImmediateContext->Map( mStagingV.Get(), 0, D3D11_MAP_READ, 0, &resV );

  if ( !mEncoder->writeFrame( (uint8_t const*)resY.pData, resY.RowPitch, (uint8_t const*)resU.pData, resU.RowPitch, (uint8_t const*)resV.pData, resV.RowPitch ) )
  {
    mEncoder->startEncoding( mRefreshRate.numer, mRefreshRate.denom );
    mEncoder->writeFrame( (uint8_t const*)resY.pData, resY.RowPitch, (uint8_t const*)resU.pData, resU.RowPitch, (uint8_t const*)resV.pData, resV.RowPitch );
  }

  mImmediateContext->Unmap( mStagingY.Get(), 0 );
  mImmediateContext->Unmap( mStagingU.Get(), 0 );
  mImmediateContext->Unmap( mStagingV.Get(), 0 );
}

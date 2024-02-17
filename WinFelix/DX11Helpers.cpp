#include "DX11Helpers.hpp"

RTVGuard::RTVGuard( ComPtr<ID3D11DeviceContext> const& pImmediateContext, ID3D11RenderTargetView* rtv ) : mImmediateContext{ pImmediateContext }
{
  mImmediateContext->OMSetRenderTargets( 1, &rtv, nullptr );
}

RTVGuard::~RTVGuard()
{
  ID3D11RenderTargetView* rtv{};
  mImmediateContext->OMSetRenderTargets( 1, &rtv, nullptr );
}

UAVGuard::UAVGuard( ComPtr<ID3D11DeviceContext> const& pImmediateContext, ID3D11UnorderedAccessView* uav ) : mImmediateContext{ pImmediateContext }, mSize{ 1 }
{
  mImmediateContext->CSSetUnorderedAccessViews( 0, 1, &uav, nullptr );
}

UAVGuard::UAVGuard( ComPtr<ID3D11DeviceContext> const& pImmediateContext, std::initializer_list<ID3D11UnorderedAccessView*> uavs ) : mImmediateContext{ pImmediateContext }, mSize{ (uint32_t)uavs.size() }
{
  assert( mSize <= UAVS );
  std::copy( uavs.begin(), uavs.end(), mUavs.begin() );
  mImmediateContext->CSSetUnorderedAccessViews( 0, mSize, mUavs.data(), nullptr );
}

UAVGuard::~UAVGuard()
{
  std::fill_n( mUavs.data(), mSize, nullptr );
  mImmediateContext->CSSetUnorderedAccessViews( 0, mSize, mUavs.data(), nullptr );
}

SRVGuard::SRVGuard( ComPtr<ID3D11DeviceContext> const& pImmediateContext, ID3D11ShaderResourceView* srv ) : mImmediateContext{ pImmediateContext }, mSize{ 1 }
{
  mImmediateContext->CSSetShaderResources( 0, 1, &srv );
}

SRVGuard::SRVGuard( ComPtr<ID3D11DeviceContext> const& pImmediateContext, std::initializer_list<ID3D11ShaderResourceView*> srvs ) : mImmediateContext{ pImmediateContext }, mSize{ (uint32_t)srvs.size() }
{
  assert( mSize <= SRVS );
  std::copy( srvs.begin(), srvs.end(), mSrvs.begin() );
  mImmediateContext->CSSetShaderResources( 0, mSize, mSrvs.data() );
}

SRVGuard::~SRVGuard()
{
  std::fill_n( mSrvs.data(), mSize, nullptr );
  mImmediateContext->CSSetShaderResources( 0, mSize, mSrvs.data() );
}

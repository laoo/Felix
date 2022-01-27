#pragma once

#include "Utility.hpp"

class RTVGuard : private NonCopyable
{
public:
  RTVGuard( ComPtr<ID3D11DeviceContext> const& pImmediateContext, ID3D11RenderTargetView* rtv );
  ~RTVGuard();

private:
  ComPtr<ID3D11DeviceContext> const& mImmediateContext;
};

class UAVGuard : private NonCopyable
{
public:
  UAVGuard( ComPtr<ID3D11DeviceContext> const& pImmediateContext, ID3D11UnorderedAccessView* uav );
  UAVGuard( ComPtr<ID3D11DeviceContext> const& pImmediateContext, std::initializer_list<ID3D11UnorderedAccessView*> uavs );
  ~UAVGuard();

private:
  static constexpr size_t UAVS = 8;

  std::array<ID3D11UnorderedAccessView*, UAVS> mUavs;
  ComPtr<ID3D11DeviceContext> const& mImmediateContext;
  uint32_t mSize;
};

class SRVGuard : private NonCopyable
{
public:
  SRVGuard( ComPtr<ID3D11DeviceContext> const& pImmediateContext, ID3D11ShaderResourceView* srv );
  SRVGuard( ComPtr<ID3D11DeviceContext> const& pImmediateContext, std::initializer_list<ID3D11ShaderResourceView*> srvs );
  ~SRVGuard();

private:
  static constexpr size_t SRVS = 8;

  std::array<ID3D11ShaderResourceView*, SRVS> mSrvs;
  ComPtr<ID3D11DeviceContext> const& mImmediateContext;
  uint32_t mSize;
};
#pragma once

#include "rational.hpp"

class IEncoder;

class EncodingRenderer
{
public:
  EncodingRenderer( std::shared_ptr<IEncoder> encoder, ComPtr<ID3D11Device> pD3DDevice, ComPtr<ID3D11DeviceContext> pImmediateContext, rational::Ratio<int32_t> refreshRate );

  void renderEncoding( ID3D11ShaderResourceView* srv );

private:

  std::shared_ptr<IEncoder> mEncoder;
  ComPtr<ID3D11Device>              mD3DDevice;
  ComPtr<ID3D11DeviceContext>       mImmediateContext;
  ComPtr<ID3D11Texture2D>           mPreStagingY;
  ComPtr<ID3D11Texture2D>           mPreStagingU;
  ComPtr<ID3D11Texture2D>           mPreStagingV;
  ComPtr<ID3D11Texture2D>           mStagingY;
  ComPtr<ID3D11Texture2D>           mStagingU;
  ComPtr<ID3D11Texture2D>           mStagingV;
  ComPtr<ID3D11UnorderedAccessView> mPreStagingYUAV;
  ComPtr<ID3D11UnorderedAccessView> mPreStagingUUAV;
  ComPtr<ID3D11UnorderedAccessView> mPreStagingVUAV;
  ComPtr<ID3D11Buffer>              mVSizeCB;
  ComPtr<ID3D11ComputeShader>       mRendererYUVCS;
  struct CBVSize
  {
    uint32_t vscale;
    uint32_t padding1;
    uint32_t padding2;
    uint32_t padding3;
  } mCb;
  rational::Ratio<int32_t> mRefreshRate;


};

#pragma once

#include "DisplayGenerator.hpp"

class WinRenderer
{
public:

  WinRenderer( HWND hWnd );
  void render( DisplayGenerator::Pixel const* surface );


private:
  struct CBPosSize
  {
    int32_t posx;
    int32_t posy;
    int32_t scalex;
    int32_t scaley;
  };

private:
  HWND mHWnd;
  ComPtr<ID3D11Device>              mD3DDevice;
  ComPtr<ID3D11DeviceContext>       mImmediateContext;
  ComPtr<IDXGISwapChain>            mSwapChain;
  ComPtr<ID3D11ComputeShader>       mRendererCS;
  ComPtr<ID3D11Buffer>              mPosSizeCB;
  ComPtr<ID3D11UnorderedAccessView> mBackBufferUAV;
  ComPtr<ID3D11Texture2D>           mSource;
  ComPtr<ID3D11ShaderResourceView>  mSourceSRV;
  int theWinWidth;
  int theWinHeight;


};

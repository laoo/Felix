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
  ATL::CComPtr<ID3D11Device>              mD3DDevice;
  ATL::CComPtr<ID3D11DeviceContext>       mImmediateContext;
  ATL::CComPtr<IDXGISwapChain>            mSwapChain;
  ATL::CComPtr<ID3D11ComputeShader>       mRendererCS;
  ATL::CComPtr<ID3D11Buffer>              mPosSizeCB;
  ATL::CComPtr<ID3D11Texture2D>           mBackBuffer;
  ATL::CComPtr<ID3D11UnorderedAccessView> mBackBufferUAV;
  int theWinWidth;
  int theWinHeight;


};

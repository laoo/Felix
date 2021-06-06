#pragma once

#include "IVideoSink.hpp"
#include "DisplayGenerator.hpp"

class WinRenderer : public IVideoSink
{
public:

  WinRenderer( HWND hWnd );
  ~WinRenderer() override = default;

  void render( DisplayGenerator::Pixel const* surface );

  DisplayLine * getNextLine( int32_t displayRow ) override;
  void updateColorReg( uint8_t value, uint8_t reg ) override;


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
  boost::rational<int32_t> mRefreshRate;
  int64_t mPerfFreq;
  int64_t mPerfCount;

};

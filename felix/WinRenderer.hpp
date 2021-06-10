#pragma once

#include "IVideoSink.hpp"
#include "DisplayGenerator.hpp"

struct RenderFrame;

class WinRenderer : public IVideoSink
{
public:

  WinRenderer( HWND hWnd );
  ~WinRenderer() override = default;

  void render() override;

  void startNewFrame() override;
  void emitScreenData( std::span<uint8_t const> data ) override;
  void updateColorReg( uint8_t reg, uint8_t value ) override;
  void endFrame() override;


private:
  struct CBPosSize
  {
    int32_t posx;
    int32_t posy;
    int32_t scalex;
    int32_t scaley;
  };

  struct Pixel
  {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t x;
  };

  struct DPixel
  {
    Pixel left;
    Pixel right;
  };


  void updatePalette( uint16_t reg, uint8_t value );
  void updateSourceTexture( std::shared_ptr<RenderFrame> frame );
  std::shared_ptr<RenderFrame> pullNextFrame();

private:
  std::array<DPixel, 256> mPalette;
  std::shared_ptr<RenderFrame> mActiveFrame;
  std::shared_ptr<RenderFrame> mFinishedFrame;
  mutable std::mutex mQueueMutex;
  uint32_t mIdx;
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

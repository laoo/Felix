#pragma once

#include "IVideoSink.hpp"
#include "DisplayGenerator.hpp"
#include "ImageCart.hpp"

struct RenderFrame;
class WinImgui;
class Manager;
class IEncoder;

class WinRenderer
{
public:

  WinRenderer();
  ~WinRenderer();

  void setEncoder( std::shared_ptr<IEncoder> encoder );
  void initialize( HWND hWnd, std::filesystem::path const& iniPath );
  int win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
  void setRotation( ImageCart::Rotation rotation );

  std::shared_ptr<IVideoSink> getVideoSink() const;

  int64_t render( Manager & config );

private:
  struct CBPosSize
  {
    int32_t posx;
    int32_t posy;
    int32_t rotx1;
    int32_t rotx2;
    int32_t roty1;
    int32_t roty2;
    int32_t size;
    uint32_t vscale;
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


  void updateSourceTexture( std::shared_ptr<RenderFrame> frame );
  void updateVscale( uint32_t vScale );

  int sizing( RECT & rect );
  bool internalRender( Manager& config );

  class SizeManager
  {
  public:
    SizeManager();
    SizeManager( int windowWidth, int windowHeight, ImageCart::Rotation rotation );

    int windowWidth() const;
    int windowHeight() const;
    int minWindowWidth() const;
    int minWindowHeight() const;
    int xOff() const;
    int yOff() const;
    int scale() const;
    int rotx1() const;
    int rotx2() const;
    int roty1() const;
    int roty2() const;
    ImageCart::Rotation rotation() const;
    explicit operator bool() const;

  private:
    int mWinWidth;
    int mWinHeight;
    int mScale;
    ImageCart::Rotation mRotation;
  };

  struct Instance : public IVideoSink
  {
    Instance();

    std::array<DPixel, 256> mPalette;
    std::shared_ptr<RenderFrame> mActiveFrame;
    std::queue<std::shared_ptr<RenderFrame>> mFinishedFrames;
    mutable std::mutex mQueueMutex;
    uint64_t mBeginTick;
    uint64_t mLastTick;
    uint64_t mFrameTicks;

    void updatePalette( uint16_t reg, uint8_t value );
    void newFrame( uint64_t tick, uint8_t hbackup ) override;
    void newRow( uint64_t tick, int row ) override;
    void emitScreenData( std::span<uint8_t const> data ) override;
    void updateColorReg( uint8_t reg, uint8_t value ) override;
    std::shared_ptr<RenderFrame> pullNextFrame();
  };

private:
  std::shared_ptr<Instance> mInstance;
  HWND mHWnd;
  std::unique_ptr<WinImgui>         mImgui;
  ComPtr<ID3D11Device>              mD3DDevice;
  ComPtr<ID3D11DeviceContext>       mImmediateContext;
  ComPtr<IDXGISwapChain>            mSwapChain;
  ComPtr<ID3D11ComputeShader>       mRendererCS;
  ComPtr<ID3D11Buffer>              mPosSizeCB;
  ComPtr<ID3D11UnorderedAccessView> mBackBufferUAV;
  ComPtr<ID3D11RenderTargetView>    mBackBufferRTV;
  ComPtr<ID3D11Texture2D>           mSource;
  ComPtr<ID3D11ShaderResourceView>  mSourceSRV;

  ComPtr<ID3D11Texture2D>           mPreStagingY;
  ComPtr<ID3D11Texture2D>           mPreStagingU;
  ComPtr<ID3D11Texture2D>           mPreStagingV;
  ComPtr<ID3D11Texture2D>           mStagingY;
  ComPtr<ID3D11Texture2D>           mStagingU;
  ComPtr<ID3D11Texture2D>           mStagingV;
  ComPtr<ID3D11UnorderedAccessView> mPreStagingYUAV;
  ComPtr<ID3D11UnorderedAccessView> mPreStagingUUAV;
  ComPtr<ID3D11UnorderedAccessView> mPreStagingVUAV;

  SizeManager mSizeManager;
  boost::rational<int32_t> mRefreshRate;
  std::shared_ptr<IEncoder> mEncoder;
  int64_t mLastRenderTimePoint;
  uint32_t mVScale;
  ImageCart::Rotation mRotation;
};

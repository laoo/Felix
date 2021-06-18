#pragma once

#include "IVideoSink.hpp"
#include "DisplayGenerator.hpp"

struct RenderFrame;
class WinImgui;
class Manager;

class WinRenderer
{
public:

  WinRenderer( int instances );
  ~WinRenderer();

  void initialize( HWND hWnd );

  std::shared_ptr<IVideoSink> getVideoSink( int instance ) const;

  void render( Manager & config );

private:
  struct CBPosSize
  {
    int32_t posx;
    int32_t posy;
    int32_t scale;
    int32_t fill;
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


  void updateSourceTexture( int instance, std::shared_ptr<RenderFrame> frame );

  friend LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
  int win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
  int sizing( RECT & rect );

  class SizeManager
  {
  public:
    SizeManager();
    SizeManager( int instances, bool horizontal, int windowWidth, int windowHeight );

    int windowWidth() const;
    int windowHeight() const;
    int minWindowWidth( std::optional<bool> horizontal = std::nullopt ) const;
    int minWindowHeight( std::optional<bool> horizontal = std::nullopt ) const;
    int instanceXOff( int instance ) const;
    int instanceYOff( int instance ) const;
    int scale() const;
    bool horizontal() const;
    explicit operator bool() const;

  private:
    int mWinWidth;
    int mWinHeight;
    int mScale;
    int mInstances;
    bool mHorizontal;
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
    void newFrame( uint64_t tick ) override;
    void newRow( uint64_t tick, int row ) override;
    void emitScreenData( std::span<uint8_t const> data ) override;
    void updateColorReg( uint8_t reg, uint8_t value ) override;
    std::shared_ptr<RenderFrame> pullNextFrame();
  };

private:
  std::vector<std::shared_ptr<Instance>> mInstances;
  HWND mHWnd;
  std::unique_ptr<WinImgui>         mImgui;
  ComPtr<ID3D11Device>              mD3DDevice;
  ComPtr<ID3D11DeviceContext>       mImmediateContext;
  ComPtr<IDXGISwapChain>            mSwapChain;
  ComPtr<ID3D11ComputeShader>       mRendererCS;
  ComPtr<ID3D11Buffer>              mPosSizeCB;
  ComPtr<ID3D11UnorderedAccessView> mBackBufferUAV;
  ComPtr<ID3D11RenderTargetView>    mBackBufferRTV;
  std::vector<ComPtr<ID3D11Texture2D>>           mSources;
  std::vector<ComPtr<ID3D11ShaderResourceView>>  mSourceSRVs;
  SizeManager mSizeManager;
  boost::rational<int32_t> mRefreshRate;
};

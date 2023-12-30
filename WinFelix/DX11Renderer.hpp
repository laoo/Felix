#pragma once
#include "Renderer.hpp"
#include "rational.hpp"

class WinImgui11;
struct VideoSink;

class DX11Renderer : public IRenderer
{
  struct Tag{};
public:
  DX11Renderer( HWND hWnd, std::filesystem::path const& iniPath, Tag );
  static std::shared_ptr<IRenderer> create( HWND hWnd, std::filesystem::path const& iniPath );

  ~DX11Renderer() override;

  int64_t render( UI& ui ) override;
  void setRotation( ImageProperties::Rotation rotation ) override;
  std::shared_ptr<IVideoSink> getVideoSink() override;

  std::shared_ptr<ICustomScreenView> makeCustomScreenView() override;
  int wndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) override;

private:

  void internalRender( UI& ui );
  bool resizeOutput();
  void updateSourceFromNextFrame();
  void renderGui( UI& ui );
  void renderScreenView( ScreenGeometry const& geometry, ID3D11ShaderResourceView* sourceSRV, ID3D11UnorderedAccessView* target );
  int sizing( RECT& rect );

private:

  class CustomScreenView : public ICustomScreenView
  {
  public:
    CustomScreenView();
    ~CustomScreenView() override = default;
    void resize( int width, int height ) override;

    void* render( std::span<uint8_t const> data, std::span<uint8_t const> palette ) override;
    void rotate( ImageProperties::Rotation rotation );
    void* getTexture() override;
    ScreenGeometry const& getGeometry() const;
    ID3D11UnorderedAccessView* getUAV();
    bool geometryChanged() const;

  private:
    void updateBuffers();

  private:
    ScreenGeometry                    mGeometry = {};
    ComPtr<ID3D11ShaderResourceView>  mSrv = {};
    ComPtr<ID3D11UnorderedAccessView> mUav = {};
    ComPtr<ID3D11Texture2D>           mSource;
    ComPtr<ID3D11ShaderResourceView>  mSourceSRV;
    ComPtr<ID3D11Buffer>              mPosSizeCB;
    std::array<uint32_t, 16>          mPalette;
    bool mGeometryChanged = {};
  };

  HWND mHWnd;
  std::shared_ptr<WinImgui11>       mImgui;
  ScreenGeometry                    mScreenGeometry;
  ImageProperties::Rotation         mRotation;

  ComPtr<IDXGISwapChain>            mSwapChain;
  ComPtr<ID3D11Buffer>              mPosSizeCB;
  ComPtr<ID3D11UnorderedAccessView> mBackBufferUAV;
  ComPtr<ID3D11RenderTargetView>    mBackBufferRTV;
  ComPtr<ID3D11Texture2D>           mSource;
  ComPtr<ID3D11ShaderResourceView>  mSourceSRV;

  rational::Ratio<int32_t>          mRefreshRate;
  std::shared_ptr<VideoSink>        mVideoSink;
  mutable std::mutex                mDebugViewMutex;
  int64_t                           mLastRenderTimePoint;
};


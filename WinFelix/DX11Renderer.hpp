#pragma once
#include "BaseRenderer.hpp"
#include "rational.hpp"

class WinImgui11;
struct VideoSink;

class DX11Renderer : public IBaseRenderer, public IExtendedRenderer
{
  struct Tag{};
public:
  DX11Renderer( HWND hWnd, std::filesystem::path const& iniPath, Tag );
  static std::pair<std::shared_ptr<IBaseRenderer>, std::shared_ptr<IExtendedRenderer>> create( HWND hWnd, std::filesystem::path const& iniPath );

  ~DX11Renderer() override;

  int64_t render( UI& ui ) override;
  void setRotation( ImageProperties::Rotation rotation ) override;
  std::shared_ptr<IVideoSink> getVideoSink() override;

  std::shared_ptr<IScreenView> makeMainScreenView() override;
  std::shared_ptr<ICustomScreenView> makeCustomScreenView() override;
  int wndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) override;

  struct BoardFont
  {
    BoardFont();
    void initialize();

    int width;
    int height;
    ComPtr<ID3D11ShaderResourceView> srv;
  };

private:
  class ScreenView;

  void internalRender( UI& ui );
  bool resizeOutput();
  void updateSourceFromNextFrame();
  void renderGui( UI& ui );
  void renderScreenView( ScreenGeometry const& geometry, ID3D11ShaderResourceView* sourceSRV, ID3D11UnorderedAccessView* target );
  bool mainScreenViewDebugRendering( std::shared_ptr<ScreenView> mainScreenView );
  int sizing( RECT& rect );

private:

  class ScreenView : public IScreenView
  {
  public:
    ScreenView();
    ~ScreenView() override = default;

    void rotate( ImageProperties::Rotation rotation );
    void resize( int width, int height ) override;
    void* getTexture() override;
    ScreenGeometry const& getGeometry() const;
    ID3D11UnorderedAccessView* getUAV();
    bool geometryChanged() const;

  protected:
    void updateBuffers();

  protected:
    ScreenGeometry mGeometry = {};
    ComPtr<ID3D11ShaderResourceView> mSrv = {};

  private:
    ComPtr<ID3D11UnorderedAccessView> mUav = {};
    bool mGeometryChanged = {};
  };

  class CustomScreenView : public ScreenView, public ICustomScreenView
  {
  public:
    CustomScreenView();
    ~CustomScreenView() override = default;
    void resize( int width, int height ) override;

    void* render( std::span<uint8_t const> data, std::span<uint8_t const> palette ) override;

  private:
    ComPtr<ID3D11Texture2D>           mSource;
    ComPtr<ID3D11ShaderResourceView>  mSourceSRV;
    ComPtr<ID3D11Texture1D>           mPalette;
    ComPtr<ID3D11ShaderResourceView>  mPaletteSRV;
    ComPtr<ID3D11Buffer>              mPosSizeCB;
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
  std::weak_ptr<ScreenView>         mMainScreenView;
  int64_t                           mLastRenderTimePoint;
};


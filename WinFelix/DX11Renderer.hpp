#pragma once
#include "BaseRenderer.hpp"

class WinImgui11;
class EncodingRenderer;

class DX11Renderer : public BaseRenderer, public IExtendedRenderer, public std::enable_shared_from_this<DX11Renderer>
{
public:
  DX11Renderer( HWND hWnd, std::filesystem::path const& iniPath );
  ~DX11Renderer() override;

  void setEncoder( std::shared_ptr<IEncoder> encoder ) override;
  std::shared_ptr<IExtendedRenderer> extendedRenderer() override;
  void* renderBoard( int id, int width, int height, std::span<uint8_t const> data ) override;

  std::shared_ptr<IScreenView> makeMainScreenView() override;
  std::shared_ptr<ICustomScreenView> makeCustomScreenView() override;

protected:

  void internalRender( UI& ui ) override;
  void present() override;
  void updateRotation() override;

private:
  class ScreenView;

  bool resizeOutput();
  void updateSourceFromNextFrame();
  void renderGui( UI& ui );
  void renderScreenView( ScreenGeometry const& geometry, ID3D11ShaderResourceView* sourceSRV, ID3D11UnorderedAccessView* target );
  bool mainScreenViewDebugRendering( std::shared_ptr<ScreenView> mainScreenView );
  static std::span<uint32_t const, 16> safePalette();

private:

  class ScreenView : public IScreenView
  {
  public:
    ScreenView();
    ~ScreenView() override = default;

    void update( ImageProperties::Rotation rotation );
    void update( int width, int height ) override;
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
    void update( int width, int height ) override;

    void* update( std::span<uint8_t const> data, std::span<uint8_t const> palette ) override;

  private:
    ComPtr<ID3D11Texture2D>           mSource;
    ComPtr<ID3D11ShaderResourceView>  mSourceSRV;
    ComPtr<ID3D11Texture1D>           mPalette;
    ComPtr<ID3D11ShaderResourceView>  mPaletteSRV;
    ComPtr<ID3D11Buffer>              mPosSizeCB;
  };

  struct Board
  {
    int width;
    int height;
    ComPtr<ID3D11Texture2D> src;
    ComPtr<ID3D11ShaderResourceView> srcSRV;
    ComPtr<ID3D11UnorderedAccessView> uav;
    ComPtr<ID3D11ShaderResourceView> srv;


    void update( DX11Renderer& r, int width, int height );
    void render( DX11Renderer& r, std::span<uint8_t const> data );
  };

  struct BoardFont
  {
    BoardFont();
    void initialize();

    int width;
    int height;
    ComPtr<ID3D11ShaderResourceView> srv;
  } mBoardFont;

  struct HexFont
  {
    HexFont();
    void initialize();

    static constexpr int width = 16;
    static constexpr int height = 16;
    static constexpr int srcWidth = 6;
    static constexpr int srcHeight = 12;

    ComPtr<ID3D11ShaderResourceView> srv;

  private:
    static uint8_t const* src( size_t idx, size_t row );
  } mHexFont;

  struct DebugRendering
  {
    int width = {};
    int height = {};
    ScreenGeometry geometry = {};
    ComPtr<ID3D11UnorderedAccessView> uav = {};
    ComPtr<ID3D11ShaderResourceView> srv = {};

    bool enabled() const;
    void update( DX11Renderer& r );
    void update( DX11Renderer& r, int width, int height );
  };

  ComPtr<IDXGISwapChain>            mSwapChain;
  ComPtr<ID3D11Buffer>              mPosSizeCB;
  ComPtr<ID3D11UnorderedAccessView> mBackBufferUAV;
  ComPtr<ID3D11RenderTargetView>    mBackBufferRTV;
  ComPtr<ID3D11Texture2D>           mSource;
  ComPtr<ID3D11ShaderResourceView>  mSourceSRV;

  boost::rational<int32_t>          mRefreshRate;
  std::shared_ptr<EncodingRenderer> mEncodingRenderer;
  std::unordered_map<int, Board>    mBoards;
  mutable std::mutex                mDebugViewMutex;
  std::weak_ptr<ScreenView>         mMainScreenView;
};


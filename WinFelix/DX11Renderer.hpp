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
  void* screenViewRenderingTexture( int id, ScreenViewType type, std::span<uint8_t const> data, std::span<uint8_t const> palette, int width, int height ) override;

  std::shared_ptr<IScreenView> makeMainScreenView() override;
  std::shared_ptr<ICustomScreenView> makeCustomScreenView( ScreenViewType type ) override;

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
    ScreenView( ComPtr<ID3D11Device> pD3DDevice, ComPtr<ID3D11DeviceContext> pImmediateContext );

    void update( ImageProperties::Rotation rotation );
    void update( int width, int height ) override;
    void* getTexture() override;
    ScreenGeometry const& getGeometry() const;
    ID3D11UnorderedAccessView* getUAV();
    bool geometryChanged() const;

  private:
    void updateBuffers();

  private:
    ComPtr<ID3D11Device>              mD3DDevice;
    ComPtr<ID3D11DeviceContext>       mImmediateContext;
    ComPtr<ID3D11UnorderedAccessView> mUav = {};
    ComPtr<ID3D11ShaderResourceView> mSrv = {};
    ScreenGeometry mGeometry = {};
    bool mGeometryChanged = {};
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
    void initialize( ID3D11Device* pDevice, ID3D11DeviceContext* pContext );

    int width;
    int height;
    ComPtr<ID3D11ShaderResourceView> srv;
  } mBoardFont;

  struct HexFont
  {
    HexFont();
    void initialize( ID3D11Device* pDevice, ID3D11DeviceContext* pContext );

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
    void render( DX11Renderer& r, ScreenViewType type, std::span<uint8_t const> data, std::span<uint8_t const> palette );
  };

  struct WindowRenderings
  {
    ComPtr<ID3D11Texture2D>           source;
    ComPtr<ID3D11ShaderResourceView>  sourceSRV;
    ComPtr<ID3D11Texture1D>           palette;
    ComPtr<ID3D11ShaderResourceView>  paletteSRV;

    std::unordered_map<int, DebugRendering> screenViews;
  } mWindowRenderings;

  ComPtr<ID3D11Device>              mD3DDevice;
  ComPtr<ID3D11DeviceContext>       mImmediateContext;
  ComPtr<IDXGISwapChain>            mSwapChain;
  ComPtr<ID3D11ComputeShader>       mRendererCS;
  ComPtr<ID3D11ComputeShader>       mRenderer2CS;
  ComPtr<ID3D11ComputeShader>       mBoardCS;
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


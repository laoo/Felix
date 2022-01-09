#pragma once
#include "BaseRenderer.hpp"

class WinImgui11;

class DX11Renderer : public BaseRenderer
{
public:
  DX11Renderer( HWND hWnd, std::filesystem::path const& iniPath );
  ~DX11Renderer() = default;
  void updateVscale( uint32_t vScale );
  bool resizeOutput();
  void updateSourceFromNextFrame();
  void renderGui( Manager& config );
  void internalRender( Manager& config );

  void renderEncoding();

  void render( Manager& config ) override;
  void setEncoder( std::shared_ptr<IEncoder> encoder ) override;
  int win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) override;
  bool canRenderBoards() const override;
  void* renderBoard( int id, int width, int height, std::span<uint8_t const> data ) override;
  void* mainRenderingTexture( int width, int height ) override;
  void* screenViewRenderingTexture( int id, ScreenViewType type, std::span<uint8_t const> data, std::span<uint8_t const> palette, int width, int height ) override;
  void renderScreenView( ScreenGeometry const& geometry, ID3D11UnorderedAccessView* target );

private:

  static std::span<uint32_t const, 16> safePalette();

  struct BoardFont;

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

  struct CBPosSize
  {
    int32_t posx;
    int32_t posy;
    int32_t rotx1;
    int32_t rotx2;
    int32_t roty1;
    int32_t roty2;
    int32_t size;
    uint32_t padding;
  };

  struct CBVSize
  {
    uint32_t vscale;
    uint32_t padding1;
    uint32_t padding2;
    uint32_t padding3;
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

    // Normal rendering but to a window
    DebugRendering main = {};
    std::unordered_map<int, DebugRendering> screenViews;
  } mWindowRenderings;


  std::shared_ptr<WinImgui11>       mImgui;
  ComPtr<ID3D11Device>              mD3DDevice;
  ComPtr<ID3D11DeviceContext>       mImmediateContext;
  ComPtr<IDXGISwapChain>            mSwapChain;
  ComPtr<ID3D11ComputeShader>       mRendererCS;
  ComPtr<ID3D11ComputeShader>       mRenderer2CS;
  ComPtr<ID3D11ComputeShader>       mRendererYUVCS;
  ComPtr<ID3D11ComputeShader>       mBoardCS;
  ComPtr<ID3D11Buffer>              mPosSizeCB;
  ComPtr<ID3D11Buffer>              mVSizeCB;
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
  boost::rational<int32_t>          mRefreshRate;
  std::shared_ptr<IEncoder>         mEncoder;
  std::unordered_map<int, Board>    mBoards;
  uint32_t mVScale;
};


#pragma once

#include "IVideoSink.hpp"
#include "DisplayGenerator.hpp"
#include "ImageCart.hpp"
#include "ImageProperties.hpp"
#include "ScreenGeometry.hpp"

class ScreenRenderingBuffer;
class WinImgui11;
class WinImgui9;
class Manager;
class IEncoder;

enum class ScreenViewType
{
  DISPADR,
  VIDBAS,
  COLLBAS,
  CUSTOM
};

class WinRenderer
{
public:

  WinRenderer();
  ~WinRenderer();

  void setEncoder( std::shared_ptr<IEncoder> encoder );
  void initialize( HWND hWnd, std::filesystem::path const& iniPath );
  int win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
  void setRotation( ImageProperties::Rotation rotation );

  std::shared_ptr<IVideoSink> getVideoSink() const;

  int64_t render( Manager & config );
  bool canRenderBoards() const;
  void* renderBoard( int id, int width, int height, std::span<uint8_t const> data );
  void* mainRenderingTexture( int width, int height );
  void* screenViewRenderingTexture( int id, ScreenViewType type, std::span<uint8_t const> data, std::span<uint8_t const> palette, int width, int height );

private:

  struct Pixel
  {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t x;
  };

  struct DPixel
  {
    Pixel left;
    Pixel right;
  };

  struct Instance : public IVideoSink
  {
    Instance();

    std::array<DPixel, 256> mPalette;
    std::shared_ptr<ScreenRenderingBuffer> mActiveFrame;
    std::queue<std::shared_ptr<ScreenRenderingBuffer>> mFinishedFrames;
    mutable std::mutex mQueueMutex;
    uint64_t mBeginTick;
    uint64_t mLastTick;
    uint64_t mFrameTicks;

    void updatePalette( uint16_t reg, uint8_t value );
    void newFrame( uint64_t tick, uint8_t hbackup ) override;
    void newRow( uint64_t tick, int row ) override;
    void emitScreenData( std::span<uint8_t const> data ) override;
    void updateColorReg( uint8_t reg, uint8_t value ) override;
    std::shared_ptr<ScreenRenderingBuffer> pullNextFrame();
  };

  class BaseRenderer
  {
  public:
    BaseRenderer( HWND hWnd );
    virtual ~BaseRenderer() = default;

    virtual void render( Manager& config ) = 0;
    virtual void setEncoder( std::shared_ptr<IEncoder> encoder ) = 0;
    virtual int win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) = 0;
    virtual void* renderBoard( int id, int width, int height, std::span<uint8_t const> data );
    virtual void* mainRenderingTexture( int width, int height );
    virtual void* screenViewRenderingTexture( int id, ScreenViewType type, std::span<uint8_t const> data, std::span<uint8_t const> palette, int width, int height );
    virtual bool canRenderBoards() const;

    std::shared_ptr<IVideoSink> getVideoSink() const;
    int sizing( RECT& rect );
    void setRotation( ImageProperties::Rotation rotation );

  protected:
    HWND mHWnd;
    std::shared_ptr<Instance> mInstance;
    ScreenGeometry mScreenGeometry;
    ImageProperties::Rotation mRotation;
  };


  class DX9Renderer : public BaseRenderer
  {
  public:
    DX9Renderer( HWND hWnd, std::filesystem::path const& iniPath );
    ~DX9Renderer() = default;
    void internalRender( Manager& config );

    void render( Manager& config ) override;
    void setEncoder( std::shared_ptr<IEncoder> encoder ) override;
    int win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) override;

  private:

    std::filesystem::path const       mIniPath;
    std::shared_ptr<WinImgui9>        mImgui;
    ComPtr<IDirect3D9Ex>              mD3D;
    ComPtr<IDirect3DDevice9Ex>        mD3Device;
    ComPtr<IDirect3DTexture9>         mSource;
    int                               mSourceWidth;
    int                               mSourceHeight;
    RECT mRect;
    std::vector<DPixel>               mTempBuffer;
  };


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

    void prepareFont();
    static std::span<uint32_t const,16> safePalette();

    struct BoardFont;

    struct Board
    {
      int width;
      int height;
      ComPtr<ID3D11Texture2D> src;
      ComPtr<ID3D11ShaderResourceView> srcSRV;
      ComPtr<ID3D11UnorderedAccessView> uav;
      ComPtr<ID3D11ShaderResourceView> srv;


      void update( WinRenderer::DX11Renderer& r, int width, int height );
      void render( WinRenderer::DX11Renderer& r, std::span<uint8_t const> data );
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
      void update( WinRenderer::DX11Renderer& r );
      void update( WinRenderer::DX11Renderer& r, int width, int height );
      void render( WinRenderer::DX11Renderer& r, ScreenViewType type, std::span<uint8_t const> data, std::span<uint8_t const> palette );
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


  std::shared_ptr<BaseRenderer> mRenderer;
  int64_t mLastRenderTimePoint;
};

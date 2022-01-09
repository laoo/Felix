#pragma once
#include "IVideoSink.hpp"
#include "ImageProperties.hpp"
#include "ScreenGeometry.hpp"

class ScreenRenderingBuffer;
class Manager;
class IEncoder;

enum class ScreenViewType
{
  DISPADR,
  VIDBAS,
  COLLBAS,
  CUSTOM
};

class BaseRenderer
{
public:


  static std::shared_ptr<BaseRenderer> createRenderer( HWND hWnd, std::filesystem::path const& iniPath );


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

  BaseRenderer( HWND hWnd );
  virtual ~BaseRenderer() = default;

  int64_t render( Manager& config );
  virtual void present() = 0;
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
  virtual void internalRender( Manager& config ) = 0;


protected:
  HWND mHWnd;
  std::shared_ptr<Instance> mInstance;
  ScreenGeometry mScreenGeometry;
  ImageProperties::Rotation mRotation;
  int64_t mLastRenderTimePoint;
};


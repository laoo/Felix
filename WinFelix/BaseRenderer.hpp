#pragma once
#include "IVideoSink.hpp"
#include "ImageProperties.hpp"
#include "ScreenGeometry.hpp"

class ScreenRenderingBuffer;
class UI;
class IEncoder;
class WinImgui;
struct VideoSink;

enum class ScreenViewType
{
  DISPADR,
  VIDBAS,
  COLLBAS,
  CUSTOM
};

class IScreenView
{
public:
  virtual ~IScreenView() = default;

  virtual void resize( int width, int height ) = 0;
  virtual void* getTexture() = 0;
};

class ICustomScreenView
{
public:
  virtual ~ICustomScreenView() = default;

  virtual void resize( int width, int height ) = 0;
  virtual void* render( std::span<uint8_t const> data, std::span<uint8_t const> palette ) = 0;
};

class IBoard
{
public:
  virtual ~IBoard() = default;

  virtual void* render( std::span<uint8_t const> data ) = 0;
  virtual void resize( int width, int height ) = 0;
};

class IExtendedRenderer
{
public:
  virtual ~IExtendedRenderer() = default;

  virtual std::shared_ptr<IScreenView> makeMainScreenView() = 0;
  virtual std::shared_ptr<ICustomScreenView> makeCustomScreenView() = 0;
  virtual std::shared_ptr<IBoard> makeBoard( int width, int height ) = 0;
  virtual void setEncoder( std::shared_ptr<IEncoder> encoder ) = 0;
};


class BaseRenderer
{
public:

  static std::shared_ptr<BaseRenderer> createRenderer( HWND hWnd, std::filesystem::path const& iniPath );

  BaseRenderer( HWND hWnd );
  virtual ~BaseRenderer();

  int64_t render( UI& ui );
  int win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

  virtual std::shared_ptr<IExtendedRenderer> extendedRenderer() = 0;

  std::shared_ptr<IVideoSink> getVideoSink() const;
  int sizing( RECT& rect );
  void setRotation( ImageProperties::Rotation rotation );

protected:
  virtual void internalRender( UI& ui ) = 0;
  virtual void present() = 0;
  virtual void updateRotation() = 0;


protected:
  HWND mHWnd;
  std::shared_ptr<WinImgui> mImgui;
  std::shared_ptr<VideoSink> mVideoSink;
  ScreenGeometry mScreenGeometry;
  ImageProperties::Rotation mRotation;
  int64_t mLastRenderTimePoint;
};


#pragma once
#include "ImageProperties.hpp"
#include "ScreenGeometry.hpp"

class UI;
class IEncoder;
struct IVideoSink;

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


class IBaseRenderer
{
public:

  virtual ~IBaseRenderer() = default;

  virtual int64_t render( UI& ui ) = 0;
  virtual void setRotation( ImageProperties::Rotation rotation ) = 0;
  virtual std::shared_ptr<IVideoSink> getVideoSink() = 0;
  virtual int wndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) = 0;

};

class ISystemDriver
{
public:
  virtual ~ISystemDriver() = default;

  static std::shared_ptr<ISystemDriver> create( HWND hWnd, std::filesystem::path const& iniPath );

  virtual std::shared_ptr<IBaseRenderer> baseRenderer() const = 0;
  virtual std::shared_ptr<IExtendedRenderer> extendedRenderer() const = 0;
  virtual int wndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) = 0;
  virtual void quit() = 0;

};


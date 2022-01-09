#pragma once

#include "IVideoSink.hpp"
#include "DisplayGenerator.hpp"
#include "ImageCart.hpp"
#include "ImageProperties.hpp"
#include "ScreenGeometry.hpp"
#include "BaseRenderer.hpp"

class ScreenRenderingBuffer;
class WinImgui11;
class WinImgui9;
class Manager;
class IEncoder;


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





  std::shared_ptr<BaseRenderer> mRenderer;
};

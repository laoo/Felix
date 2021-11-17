#pragma once

#include "imgui.h"

class WinImgui
{
public:

  enum class TextureFormat
  {
    BGRA = 0,
    RGBA = 1
  };

  WinImgui( HWND hWnd, std::filesystem::path const& iniPath );
  virtual ~WinImgui();

  void win32_NewFrame();

  int win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

  virtual void* createTextureRaw( uint8_t const* textureData, int width, int height, TextureFormat fmt ) = 0;
  virtual void deleteTextureRaw( void* textureData ) = 0;



protected:
  void win32_UpdateMousePos();
  bool win32_UpdateMouseCursor();

protected:

  HWND mhWnd;

  INT64                mTime;
  INT64                mTicksPerSecond;
  ImGuiMouseCursor     mLastMouseCursor;

  std::string mIniFilePath;
};

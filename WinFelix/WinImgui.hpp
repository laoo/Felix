#pragma once

#include "imgui.h"

class WinImgui
{
public:

  WinImgui( HWND hWnd, std::filesystem::path const& iniPath );
  virtual ~WinImgui();

  void newFrame();
  virtual void renderNewFrame() = 0;
  virtual void renderDrawData( ImDrawData* draw_data ) = 0;

  int win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

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

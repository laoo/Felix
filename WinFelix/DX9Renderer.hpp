#pragma once
#include "BaseRenderer.hpp"

class WinImgui9;

class DX9Renderer : public BaseRenderer
{
public:
  DX9Renderer( HWND hWnd, std::filesystem::path const& iniPath );
  ~DX9Renderer() = default;

  void present() override;
  void setEncoder( std::shared_ptr<IEncoder> encoder ) override;
  int win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) override;

protected:
  void internalRender( Manager& config ) override;


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

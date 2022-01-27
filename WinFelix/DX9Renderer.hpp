#pragma once
#include "BaseRenderer.hpp"

class WinImgui9;

class DX9Renderer : public BaseRenderer
{
public:
  DX9Renderer( HWND hWnd, std::filesystem::path const& iniPath );
  ~DX9Renderer() override;

  std::shared_ptr<IExtendedRenderer> extendedRenderer() override;


protected:
  void internalRender( UI& ui ) override;
  void present() override;
  void updateRotation() override;


private:

  std::filesystem::path const       mIniPath;
  ComPtr<IDirect3D9Ex>              mD3D;
  ComPtr<IDirect3DDevice9Ex>        mD3Device;
  ComPtr<IDirect3DTexture9>         mSource;
  int                               mSourceWidth;
  int                               mSourceHeight;
  RECT mRect;
  std::vector<uint64_t>             mTempBuffer;
};

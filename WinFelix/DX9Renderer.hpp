#pragma once
#include "BaseRenderer.hpp"

class WinImgui9;
struct VideoSink;

class DX9Renderer : public IBaseRenderer
{
public:
  DX9Renderer( HWND hWnd, std::filesystem::path const& iniPath );
  ~DX9Renderer() override;

  std::shared_ptr<IExtendedRenderer> extendedRenderer() override;
  int64_t render( UI& ui ) override;
  void setRotation( ImageProperties::Rotation rotation ) override;
  std::shared_ptr<IVideoSink> getVideoSink() override;
  int wndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) override;

private:
  void internalRender( UI& ui );
  int sizing( RECT& rect );

private:

  HWND                              mHWnd;
  std::shared_ptr<WinImgui9>        mImgui;
  ScreenGeometry                    mScreenGeometry;
  ImageProperties::Rotation         mRotation;
  std::filesystem::path const       mIniPath;
  std::shared_ptr<VideoSink>        mVideoSink;
  ComPtr<IDirect3D9Ex>              mD3D;
  ComPtr<IDirect3DDevice9Ex>        mD3Device;
  ComPtr<IDirect3DTexture9>         mSource;
  int                               mSourceWidth;
  int                               mSourceHeight;
  RECT mRect;
  std::vector<uint64_t>             mTempBuffer;
  int64_t mLastRenderTimePoint;
};

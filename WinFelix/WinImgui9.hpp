#pragma once

#include "WinImgui.hpp"

class WinImgui9 : public WinImgui
{
public:
  WinImgui9( HWND hWnd, ComPtr<IDirect3DDevice9> pD3DDevice, std::filesystem::path const& iniPath );
  ~WinImgui9() override;

  void renderNewFrame() override;
  void renderDrawData( ImDrawData* draw_data ) override;

private:
  void createFontsTexture();
  void setupRenderState( ImDrawData* draw_data );


private:

  int mVertexBufferSize;
  int mIndexBufferSize;

  struct CUSTOMVERTEX
  {
    float    pos[3];
    D3DCOLOR col;
    float    uv[2];
  };

  ComPtr<IDirect3DDevice9>                md3dDevice;
  ComPtr<IDirect3DVertexBuffer9>          mVB;
  ComPtr<IDirect3DIndexBuffer9>           mIB;
  ComPtr<IDirect3DTexture9>               mFontTexture;

};

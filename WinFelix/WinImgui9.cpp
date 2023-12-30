#include "WinImgui9.hpp"

#define V_THROW(x) { HRESULT hr_ = (x); if( FAILED( hr_ ) ) { throw std::runtime_error{ "DXError" }; } }

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)

#ifdef IMGUI_USE_BGRA_PACKED_COLOR
#define IMGUI_COL_TO_DX9_ARGB(_COL)     (_COL)
#else
#define IMGUI_COL_TO_DX9_ARGB(_COL)     (((_COL) & 0xFF00FF00) | (((_COL) & 0xFF0000) >> 16) | (((_COL) & 0xFF) << 16))
#endif

WinImgui9::WinImgui9( HWND hWnd, ComPtr<IDirect3DDevice9> pD3DDevice, std::filesystem::path const& iniPath ) : WinImgui{ hWnd, iniPath },
  md3dDevice{ std::move( pD3DDevice ) }, mVertexBufferSize{ 5000 }, mIndexBufferSize{ 10000 }
{
}

WinImgui9::~WinImgui9()
{
}

void WinImgui9::createFontsTexture()
{
  // Build texture atlas
  ImGuiIO& io = ImGui::GetIO();
  unsigned char* pixels;
  int width, height, bytes_per_pixel;
  io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height, &bytes_per_pixel );

  // Convert RGBA32 to BGRA32 (because RGBA32 is not well supported by DX9 devices)
#ifndef IMGUI_USE_BGRA_PACKED_COLOR
  if ( io.Fonts->TexPixelsUseColors )
  {
    ImU32* dst_start = (ImU32*)ImGui::MemAlloc( (size_t)width * height * bytes_per_pixel );
    for ( ImU32* src = (ImU32*)pixels, *dst = dst_start, *dst_end = dst_start + (size_t)width * height; dst < dst_end; src++, dst++ )
      *dst = IMGUI_COL_TO_DX9_ARGB( *src );
    pixels = (unsigned char*)dst_start;
  }
#endif

  // Upload texture to graphics system
  V_THROW( md3dDevice->CreateTexture( width, height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, mFontTexture.ReleaseAndGetAddressOf(), NULL ) );
  D3DLOCKED_RECT tex_locked_rect;
  V_THROW( mFontTexture->LockRect( 0, &tex_locked_rect, NULL, 0 ) );
  for ( int y = 0; y < height; y++ )
    memcpy( (unsigned char*)tex_locked_rect.pBits + (size_t)tex_locked_rect.Pitch * y, pixels + (size_t)width * bytes_per_pixel * y, (size_t)width * bytes_per_pixel );
  mFontTexture->UnlockRect( 0 );

  // Store our identifier
  io.Fonts->SetTexID( (ImTextureID)mFontTexture.Get() );

#ifndef IMGUI_USE_BGRA_PACKED_COLOR
  if ( io.Fonts->TexPixelsUseColors )
    ImGui::MemFree( pixels );
#endif
}

void WinImgui9::setupRenderState( ImDrawData* draw_data )
{
  // Setup viewport
  D3DVIEWPORT9 vp;
  vp.X = vp.Y = 0;
  vp.Width = (DWORD)draw_data->DisplaySize.x;
  vp.Height = (DWORD)draw_data->DisplaySize.y;
  vp.MinZ = 0.0f;
  vp.MaxZ = 1.0f;
  md3dDevice->SetViewport( &vp );

  // Setup render state: fixed-pipeline, alpha-blending, no face culling, no depth testing, shade mode (for gradient)
  md3dDevice->SetPixelShader( NULL );
  md3dDevice->SetVertexShader( NULL );
  md3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  md3dDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
  md3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
  md3dDevice->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );
  md3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
  md3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
  md3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  md3dDevice->SetRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );
  md3dDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
  md3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
  md3dDevice->SetRenderState( D3DRS_SEPARATEALPHABLENDENABLE, TRUE );
  md3dDevice->SetRenderState( D3DRS_SRCBLENDALPHA, D3DBLEND_ONE );
  md3dDevice->SetRenderState( D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA );
  md3dDevice->SetRenderState( D3DRS_SCISSORTESTENABLE, TRUE );
  md3dDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
  md3dDevice->SetRenderState( D3DRS_RANGEFOGENABLE, FALSE );
  md3dDevice->SetRenderState( D3DRS_SPECULARENABLE, FALSE );
  md3dDevice->SetRenderState( D3DRS_STENCILENABLE, FALSE );
  md3dDevice->SetRenderState( D3DRS_CLIPPING, TRUE );
  md3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
  md3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
  md3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  md3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
  md3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
  md3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
  md3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
  md3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
  md3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
  md3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
  md3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );

  // Setup orthographic projection matrix
  // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
  // Being agnostic of whether <d3dx9.h> or <DirectXMath.h> can be used, we aren't relying on D3DXMatrixIdentity()/D3DXMatrixOrthoOffCenterLH() or DirectX::XMMatrixIdentity()/DirectX::XMMatrixOrthographicOffCenterLH()
  {
    float L = draw_data->DisplayPos.x + 0.5f;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x + 0.5f;
    float T = draw_data->DisplayPos.y + 0.5f;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y + 0.5f;
    D3DMATRIX mat_identity = { { { 1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f } } };
    D3DMATRIX mat_projection =
    { { {
        2.0f / ( R - L ),   0.0f,         0.0f,  0.0f,
        0.0f,         2.0f / ( T - B ),   0.0f,  0.0f,
        0.0f,         0.0f,         0.5f,  0.0f,
        ( L + R ) / ( L - R ),  ( T + B ) / ( B - T ),  0.5f,  1.0f
    } } };
    md3dDevice->SetTransform( D3DTS_WORLD, &mat_identity );
    md3dDevice->SetTransform( D3DTS_VIEW, &mat_identity );
    md3dDevice->SetTransform( D3DTS_PROJECTION, &mat_projection );
  }
}

void WinImgui9::renderNewFrame()
{
  if ( !md3dDevice )
    throw std::exception{};

  if ( !mFontTexture )
    createFontsTexture();
}

void WinImgui9::renderDrawData( ImDrawData* draw_data )
{
  // Avoid rendering when minimized
  if ( draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f )
    return;

  if ( !mVB || mVertexBufferSize < draw_data->TotalVtxCount )
  {
    mVertexBufferSize = draw_data->TotalVtxCount + 5000;
    V_THROW( md3dDevice->CreateVertexBuffer( mVertexBufferSize * sizeof( CUSTOMVERTEX ), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, mVB.ReleaseAndGetAddressOf(), NULL ) );
  }
  if ( !mIB || mIndexBufferSize < draw_data->TotalIdxCount )
  {
    mIndexBufferSize = draw_data->TotalIdxCount + 10000;
    V_THROW( md3dDevice->CreateIndexBuffer( mIndexBufferSize * sizeof( ImDrawIdx ), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, sizeof( ImDrawIdx ) == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32, D3DPOOL_DEFAULT, mIB.ReleaseAndGetAddressOf(), NULL ) );
  }

  // Backup the DX9 state
  ComPtr<IDirect3DStateBlock9> d3d9_state_block;
  V_THROW( md3dDevice->CreateStateBlock( D3DSBT_ALL, d3d9_state_block.ReleaseAndGetAddressOf() ) );
  V_THROW( d3d9_state_block->Capture() );

  // Backup the DX9 transform (DX9 documentation suggests that it is included in the StateBlock but it doesn't appear to)
  D3DMATRIX last_world, last_view, last_projection;
  md3dDevice->GetTransform( D3DTS_WORLD, &last_world );
  md3dDevice->GetTransform( D3DTS_VIEW, &last_view );
  md3dDevice->GetTransform( D3DTS_PROJECTION, &last_projection );

  {
    // Allocate buffers
    CUSTOMVERTEX* vtx_dst;
    ImDrawIdx* idx_dst;
    V_THROW( mVB->Lock( 0, (UINT)( draw_data->TotalVtxCount * sizeof( CUSTOMVERTEX ) ), (void**)&vtx_dst, D3DLOCK_DISCARD ) );
    V_THROW( mIB->Lock( 0, (UINT)( draw_data->TotalIdxCount * sizeof( ImDrawIdx ) ), (void**)&idx_dst, D3DLOCK_DISCARD ) );

    // Copy and convert all vertices into a single contiguous buffer, convert colors to DX9 default format.
    // FIXME-OPT: This is a minor waste of resource, the ideal is to use imconfig.h and
    //  1) to avoid repacking colors:   #define IMGUI_USE_BGRA_PACKED_COLOR
    //  2) to avoid repacking vertices: #define IMGUI_OVERRIDE_DRAWVERT_STRUCT_LAYOUT struct ImDrawVert { ImVec2 pos; float z; ImU32 col; ImVec2 uv; }
    for ( int n = 0; n < draw_data->CmdListsCount; n++ )
    {
      const ImDrawList* cmd_list = draw_data->CmdLists[n];
      const ImDrawVert* vtx_src = cmd_list->VtxBuffer.Data;
      for ( int i = 0; i < cmd_list->VtxBuffer.Size; i++ )
      {
        vtx_dst->pos[0] = vtx_src->pos.x;
        vtx_dst->pos[1] = vtx_src->pos.y;
        vtx_dst->pos[2] = 0.0f;
        vtx_dst->col = IMGUI_COL_TO_DX9_ARGB( vtx_src->col );
        vtx_dst->uv[0] = vtx_src->uv.x;
        vtx_dst->uv[1] = vtx_src->uv.y;
        vtx_dst++;
        vtx_src++;
      }
      memcpy( idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof( ImDrawIdx ) );
      idx_dst += cmd_list->IdxBuffer.Size;
    }
    mIB->Unlock();
    mVB->Unlock();
  }
  md3dDevice->SetStreamSource( 0, mVB.Get(), 0, sizeof( CUSTOMVERTEX ) );
  md3dDevice->SetIndices( mIB.Get() );
  md3dDevice->SetFVF( D3DFVF_CUSTOMVERTEX );

  // Setup desired DX state
  setupRenderState( draw_data );

  // Render command lists
  // (Because we merged all buffers into a single one, we maintain our own offset into them)
  int global_vtx_offset = 0;
  int global_idx_offset = 0;
  ImVec2 clip_off = draw_data->DisplayPos;
  for ( int n = 0; n < draw_data->CmdListsCount; n++ )
  {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    for ( int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++ )
    {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
      if ( pcmd->UserCallback != NULL )
      {
        // User callback, registered via ImDrawList::AddCallback()
        // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
        if ( pcmd->UserCallback == ImDrawCallback_ResetRenderState )
          setupRenderState( draw_data );
        else
          pcmd->UserCallback( cmd_list, pcmd );
      }
      else
      {
        // Project scissor/clipping rectangles into framebuffer space
        ImVec2 clip_min( pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y );
        ImVec2 clip_max( pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y );
        if ( clip_max.x < clip_min.x || clip_max.y < clip_min.y )
          continue;

        // Apply Scissor/clipping rectangle, Bind texture, Draw
        const RECT r = { (LONG)clip_min.x, (LONG)clip_min.y, (LONG)clip_max.x, (LONG)clip_max.y };
        const LPDIRECT3DTEXTURE9 texture = (LPDIRECT3DTEXTURE9)pcmd->GetTexID();
        md3dDevice->SetTexture( 0, texture );
        md3dDevice->SetScissorRect( &r );
        md3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, pcmd->VtxOffset + global_vtx_offset, 0, (UINT)cmd_list->VtxBuffer.Size, pcmd->IdxOffset + global_idx_offset, pcmd->ElemCount / 3 );
      }
    }
    global_idx_offset += cmd_list->IdxBuffer.Size;
    global_vtx_offset += cmd_list->VtxBuffer.Size;
  }

  // Restore the DX9 transform
  md3dDevice->SetTransform( D3DTS_WORLD, &last_world );
  md3dDevice->SetTransform( D3DTS_VIEW, &last_view );
  md3dDevice->SetTransform( D3DTS_PROJECTION, &last_projection );

  // Restore the DX9 state
  d3d9_state_block->Apply();
  d3d9_state_block->Release();
}

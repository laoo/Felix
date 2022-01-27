#include "pch.hpp"
#include "WinImgui11.hpp"
#include "vertex.hxx"
#include "pixel.hxx"

WinImgui11::WinImgui11( HWND hWnd, ComPtr<ID3D11Device> pD3DDevice, ComPtr<ID3D11DeviceContext> pDeviceContext, std::filesystem::path const& iniPath ) : WinImgui{ hWnd, iniPath },
  md3dDevice{ std::move( pD3DDevice ) }, md3dDeviceContext{ std::move( pDeviceContext ) }, mVertexBufferSize{ 5000 }, mIndexBufferSize{ 10000 }
{
}

WinImgui11::~WinImgui11()
{
}


void WinImgui11::renderNewFrame()
{
  if ( mFontSampler )
    return;

  if ( !md3dDevice )
    throw std::exception{};

  // Create the vertex shader
  {
    if ( md3dDevice->CreateVertexShader( (DWORD*)g_Vertex, sizeof g_Vertex, NULL, mVertexShader.ReleaseAndGetAddressOf() ) != S_OK )
      throw std::exception{};

    // Create the input layout
    D3D11_INPUT_ELEMENT_DESC local_layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)( &( (ImDrawVert*)0 )->pos ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)( &( (ImDrawVert*)0 )->uv ),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (size_t)( &( (ImDrawVert*)0 )->col ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    if ( md3dDevice->CreateInputLayout( local_layout, 3, g_Vertex, sizeof g_Vertex, mInputLayout.ReleaseAndGetAddressOf() ) != S_OK )
      throw std::exception{};

    // Create the constant buffer
    {
      D3D11_BUFFER_DESC desc;
      desc.ByteWidth = sizeof( VERTEX_CONSTANT_BUFFER );
      desc.Usage = D3D11_USAGE_DYNAMIC;
      desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      desc.MiscFlags = 0;
      md3dDevice->CreateBuffer( &desc, NULL, mVertexConstantBuffer.ReleaseAndGetAddressOf() );
    }
  }

  if ( md3dDevice->CreatePixelShader( (DWORD*)g_Pixel, sizeof g_Pixel, NULL, mPixelShader.ReleaseAndGetAddressOf() ) != S_OK )
    throw std::exception{};

  // Create the blending setup
  {
    D3D11_BLEND_DESC desc;
    ZeroMemory( &desc, sizeof( desc ) );
    desc.AlphaToCoverageEnable = false;
    desc.RenderTarget[0].BlendEnable = true;
    desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    md3dDevice->CreateBlendState( &desc, mBlendState.ReleaseAndGetAddressOf() );
  }

  // Create the rasterizer state
  {
    D3D11_RASTERIZER_DESC desc;
    ZeroMemory( &desc, sizeof( desc ) );
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_NONE;
    desc.ScissorEnable = true;
    desc.DepthClipEnable = true;
    md3dDevice->CreateRasterizerState( &desc, mRasterizerState.ReleaseAndGetAddressOf() );
  }

  // Create depth-stencil State
  {
    D3D11_DEPTH_STENCIL_DESC desc;
    ZeroMemory( &desc, sizeof( desc ) );
    desc.DepthEnable = false;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    desc.StencilEnable = false;
    desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    desc.BackFace = desc.FrontFace;
    md3dDevice->CreateDepthStencilState( &desc, mDepthStencilState.ReleaseAndGetAddressOf() );
  }

  createFontsTexture();
}

void WinImgui11::renderDrawData( ImDrawData * draw_data )
{
  // Avoid rendering when minimized
  if ( draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f )
    return;

  ID3D11DeviceContext* ctx = md3dDeviceContext.Get();

  // Create and grow vertex/index buffers if needed
  if ( !mVB || mVertexBufferSize < draw_data->TotalVtxCount )
  {
    mVertexBufferSize = draw_data->TotalVtxCount + 5000;
    D3D11_BUFFER_DESC desc;
    memset( &desc, 0, sizeof( D3D11_BUFFER_DESC ) );
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = mVertexBufferSize * sizeof( ImDrawVert );
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    if ( md3dDevice->CreateBuffer( &desc, NULL, mVB.ReleaseAndGetAddressOf() ) < 0 )
      return;
  }
  if ( !mIB || mIndexBufferSize < draw_data->TotalIdxCount )
  {
    mIndexBufferSize = draw_data->TotalIdxCount + 10000;
    D3D11_BUFFER_DESC desc;
    memset( &desc, 0, sizeof( D3D11_BUFFER_DESC ) );
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = mIndexBufferSize * sizeof( ImDrawIdx );
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if ( md3dDevice->CreateBuffer( &desc, NULL, mIB.ReleaseAndGetAddressOf() ) < 0 )
      return;
  }

  // Upload vertex/index data into a single contiguous GPU buffer
  D3D11_MAPPED_SUBRESOURCE vtx_resource, idx_resource;
  if ( ctx->Map( mVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &vtx_resource ) != S_OK )
    return;
  if ( ctx->Map( mIB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource ) != S_OK )
    return;
  ImDrawVert* vtx_dst = (ImDrawVert*)vtx_resource.pData;
  ImDrawIdx* idx_dst = (ImDrawIdx*)idx_resource.pData;
  for ( int n = 0; n < draw_data->CmdListsCount; n++ )
  {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    memcpy( vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof( ImDrawVert ) );
    memcpy( idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof( ImDrawIdx ) );
    vtx_dst += cmd_list->VtxBuffer.Size;
    idx_dst += cmd_list->IdxBuffer.Size;
  }
  ctx->Unmap( mVB.Get(), 0 );
  ctx->Unmap( mIB.Get(), 0 );

  // Setup orthographic projection matrix into our constant buffer
  // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
  {
    D3D11_MAPPED_SUBRESOURCE mapped_resource;
    if ( ctx->Map( mVertexConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource ) != S_OK )
      return;
    VERTEX_CONSTANT_BUFFER* constant_buffer = (VERTEX_CONSTANT_BUFFER*)mapped_resource.pData;
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    float mvp[4][4] =
    {
        { 2.0f / ( R - L ),   0.0f,           0.0f,       0.0f },
        { 0.0f,         2.0f / ( T - B ),     0.0f,       0.0f },
        { 0.0f,         0.0f,           0.5f,       0.0f },
        { ( R + L ) / ( L - R ),  ( T + B ) / ( B - T ),    0.5f,       1.0f },
    };
    memcpy( &constant_buffer->mvp, mvp, sizeof( mvp ) );
    ctx->Unmap( mVertexConstantBuffer.Get(), 0 );
  }

  // Setup desired DX state
  setupRenderState( draw_data, ctx );

  // Render command lists
  // (Because we merged all buffers into a single one, we maintain our own offset into them)
  int global_idx_offset = 0;
  int global_vtx_offset = 0;
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
          setupRenderState( draw_data, ctx );
        else
          pcmd->UserCallback( cmd_list, pcmd );
      }
      else
      {
        // Apply scissor/clipping rectangle
        const D3D11_RECT r ={ (LONG)( pcmd->ClipRect.x - clip_off.x ), (LONG)( pcmd->ClipRect.y - clip_off.y ), (LONG)( pcmd->ClipRect.z - clip_off.x ), (LONG)( pcmd->ClipRect.w - clip_off.y ) };
        ctx->RSSetScissorRects( 1, &r );

        // Bind texture, Draw
        ID3D11ShaderResourceView* texture_srv = (ID3D11ShaderResourceView*)pcmd->TextureId;
        ctx->PSSetShaderResources( 0, 1, &texture_srv );
        ctx->DrawIndexed( pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset );
      }
    }
    global_idx_offset += cmd_list->IdxBuffer.Size;
    global_vtx_offset += cmd_list->VtxBuffer.Size;
  }
  ID3D11ShaderResourceView* nullTex{};
  ctx->PSSetShaderResources( 0, 1, &nullTex );
}

void WinImgui11::setupRenderState( ImDrawData* draw_data, ID3D11DeviceContext* ctx )
{
  // Setup viewport
  D3D11_VIEWPORT vp;
  memset( &vp, 0, sizeof( D3D11_VIEWPORT ) );
  vp.Width = draw_data->DisplaySize.x;
  vp.Height = draw_data->DisplaySize.y;
  vp.MinDepth = 0.0f;
  vp.MaxDepth = 1.0f;
  vp.TopLeftX = vp.TopLeftY = 0;
  ctx->RSSetViewports( 1, &vp );

  // Setup shader and vertex buffers
  unsigned int stride = sizeof( ImDrawVert );
  unsigned int offset = 0;
  ctx->IASetInputLayout( mInputLayout.Get() );
  ctx->IASetVertexBuffers( 0, 1, mVB.GetAddressOf(), &stride, &offset );
  ctx->IASetIndexBuffer( mIB.Get(), sizeof( ImDrawIdx ) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0 );
  ctx->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
  ctx->VSSetShader( mVertexShader.Get(), NULL, 0 );
  ctx->VSSetConstantBuffers( 0, 1, mVertexConstantBuffer.GetAddressOf() );
  ctx->PSSetShader( mPixelShader.Get(), NULL, 0 );
  ctx->PSSetSamplers( 0, 1, mFontSampler.GetAddressOf() );
  ctx->GSSetShader( NULL, NULL, 0 );
  ctx->HSSetShader( NULL, NULL, 0 ); // In theory we should backup and restore this as well.. very infrequently used..
  ctx->DSSetShader( NULL, NULL, 0 ); // In theory we should backup and restore this as well.. very infrequently used..
  ctx->CSSetShader( NULL, NULL, 0 ); // In theory we should backup and restore this as well.. very infrequently used..

  // Setup blend state
  const float blend_factor[4] ={ 0.f, 0.f, 0.f, 0.f };
  ctx->OMSetBlendState( mBlendState.Get(), blend_factor, 0xffffffff );
  ctx->OMSetDepthStencilState( mDepthStencilState.Get(), 0 );
  ctx->RSSetState( mRasterizerState.Get() );
}

void WinImgui11::createFontsTexture()
{
  // Build texture atlas
  ImGuiIO& io = ImGui::GetIO();
  unsigned char* pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height );

  // Upload texture to graphics system
  {
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory( &desc, sizeof( desc ) );
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D *pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = pixels;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    md3dDevice->CreateTexture2D( &desc, &subResource, &pTexture );

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory( &srvDesc, sizeof( srvDesc ) );
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    md3dDevice->CreateShaderResourceView( pTexture, &srvDesc, mFontTextureView.ReleaseAndGetAddressOf() );
    pTexture->Release();
  }

  // Store our identifier
  io.Fonts->TexID = (ImTextureID)mFontTextureView.Get();

  // Create texture sampler
  {
    D3D11_SAMPLER_DESC desc;
    ZeroMemory( &desc, sizeof( desc ) );
    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.MipLODBias = 0.f;
    desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    desc.MinLOD = 0.f;
    desc.MaxLOD = 0.f;
    md3dDevice->CreateSamplerState( &desc, mFontSampler.ReleaseAndGetAddressOf() );
  }

}

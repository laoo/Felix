#include "pch.hpp"
#include "imgui.h"
#include "WinImgui.hpp"
#ifdef _MSC_VER
#pragma comment(lib, "d3dcompiler") // Automatically link with d3dcompiler.lib as we are using D3DCompile() below.
#endif

WinImgui::WinImgui( HWND hWnd, ComPtr<ID3D11Device> pD3DDevice, ComPtr<ID3D11DeviceContext> pDeviceContext ) : mhWnd{ hWnd }, md3dDevice{ std::move( pD3DDevice) }, md3dDeviceContext{ std::move( pDeviceContext ) },
  mTime{}, mTicksPerSecond{}, mLastMouseCursor{ ImGuiMouseCursor_COUNT },
  mVertexBufferSize{ 5000 }, mIndexBufferSize{ 10000 }
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  if ( !::QueryPerformanceFrequency( (LARGE_INTEGER *)&mTicksPerSecond ) )
    throw std::exception{};
  if ( !::QueryPerformanceCounter( (LARGE_INTEGER *)&mTime ) )
    throw std::exception{};

  ImGuiIO& io = ImGui::GetIO();
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
  io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
  io.BackendPlatformName = "imgui_impl_win32";
  io.ImeWindowHandle = mhWnd;

  // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array that we will update during the application lifetime.
  io.KeyMap[ImGuiKey_Tab] = VK_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
  io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
  io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
  io.KeyMap[ImGuiKey_Home] = VK_HOME;
  io.KeyMap[ImGuiKey_End] = VK_END;
  io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
  io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
  io.KeyMap[ImGuiKey_Space] = VK_SPACE;
  io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
  io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
  io.KeyMap[ImGuiKey_KeyPadEnter] = VK_RETURN;
  io.KeyMap[ImGuiKey_A] = 'A';
  io.KeyMap[ImGuiKey_C] = 'C';
  io.KeyMap[ImGuiKey_V] = 'V';
  io.KeyMap[ImGuiKey_X] = 'X';
  io.KeyMap[ImGuiKey_Y] = 'Y';
  io.KeyMap[ImGuiKey_Z] = 'Z';

  io.BackendRendererName = "imgui_impl_dx11";
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

  ImGui::StyleColorsLight();
}

WinImgui::~WinImgui()
{
}


void WinImgui::win32_NewFrame()
{
  ImGuiIO& io = ImGui::GetIO();
  IM_ASSERT( io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame()." );

  // Setup display size (every frame to accommodate for window resizing)
  RECT rect;
  ::GetClientRect( mhWnd, &rect );
  io.DisplaySize = ImVec2( (float)( rect.right - rect.left ), (float)( rect.bottom - rect.top ) );

  // Setup time step
  INT64 current_time;
  ::QueryPerformanceCounter( (LARGE_INTEGER *)&current_time );
  io.DeltaTime = (float)( current_time - mTime ) / mTicksPerSecond;
  mTime = current_time;

  // Read keyboard modifiers inputs
  io.KeyCtrl = ( ::GetKeyState( VK_CONTROL ) & 0x8000 ) != 0;
  io.KeyShift = ( ::GetKeyState( VK_SHIFT ) & 0x8000 ) != 0;
  io.KeyAlt = ( ::GetKeyState( VK_MENU ) & 0x8000 ) != 0;
  io.KeySuper = false;
  // io.KeysDown[], io.MousePos, io.MouseDown[], io.MouseWheel: filled by the WndProc handler below.

  // Update OS mouse position
  win32_UpdateMousePos();

  // Update OS mouse cursor with the cursor requested by imgui
  ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
  if ( mLastMouseCursor != mouse_cursor )
  {
    mLastMouseCursor = mouse_cursor;
    win32_UpdateMouseCursor();
  }
}

void WinImgui::dx11_NewFrame()
{
  if ( mFontSampler )
    return;

  if ( !md3dDevice )
    throw std::exception{};

  // By using D3DCompile() from <d3dcompiler.h> / d3dcompiler.lib, we introduce a dependency to a given version of d3dcompiler_XX.dll (see D3DCOMPILER_DLL_A)
  // If you would like to use this DX11 sample code but remove this dependency you can:
  //  1) compile once, save the compiled shader blobs into a file or source code and pass them to CreateVertexShader()/CreatePixelShader() [preferred solution]
  //  2) use code to detect any version of the DLL and grab a pointer to D3DCompile from the DLL.
  // See https://github.com/ocornut/imgui/pull/638 for sources and details.

  // Create the vertex shader
  {
    static const char* vertexShader =
      "cbuffer vertexBuffer : register(b0) \
            {\
            float4x4 ProjectionMatrix; \
            };\
            struct VS_INPUT\
            {\
            float2 pos : POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            \
            struct PS_INPUT\
            {\
            float4 pos : SV_POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            \
            PS_INPUT main(VS_INPUT input)\
            {\
            PS_INPUT output;\
            output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
            output.col = input.col;\
            output.uv  = input.uv;\
            return output;\
            }";

    ComPtr<ID3D10Blob> vertexShaderBlob;
    D3DCompile( vertexShader, strlen( vertexShader ), NULL, NULL, NULL, "main", "vs_4_0", 0, 0, vertexShaderBlob.ReleaseAndGetAddressOf(), NULL );
    if ( !vertexShaderBlob ) // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
      throw std::exception{};
    if ( md3dDevice->CreateVertexShader( (DWORD*)vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), NULL, mVertexShader.ReleaseAndGetAddressOf() ) != S_OK )
      throw std::exception{};

    // Create the input layout
    D3D11_INPUT_ELEMENT_DESC local_layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)( &( (ImDrawVert*)0 )->pos ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)( &( (ImDrawVert*)0 )->uv ),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (size_t)( &( (ImDrawVert*)0 )->col ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    if ( md3dDevice->CreateInputLayout( local_layout, 3, vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), mInputLayout.ReleaseAndGetAddressOf() ) != S_OK )
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

  // Create the pixel shader
  {
    static const char* pixelShader =
      "struct PS_INPUT\
            {\
            float4 pos : SV_POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            sampler sampler0;\
            Texture2D texture0;\
            \
            float4 main(PS_INPUT input) : SV_Target\
            {\
            float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
            return out_col; \
            }";

    ComPtr<ID3D10Blob> pixelShaderBlob;
    D3DCompile( pixelShader, strlen( pixelShader ), NULL, NULL, NULL, "main", "ps_4_0", 0, 0, pixelShaderBlob.ReleaseAndGetAddressOf(), NULL );
    if ( !pixelShaderBlob )  // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
      throw std::exception{};
    if ( md3dDevice->CreatePixelShader( (DWORD*)pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), NULL, mPixelShader.ReleaseAndGetAddressOf() ) != S_OK )
      throw std::exception{};
  }

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

  dx11_CreateFontsTexture();
}

void WinImgui::dx11_RenderDrawData( ImDrawData * draw_data )
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

  // Backup DX state that will be modified to restore it afterwards (unfortunately this is very ugly looking and verbose. Close your eyes!)
  struct BACKUP_DX11_STATE
  {
    UINT                        ScissorRectsCount, ViewportsCount;
    D3D11_RECT                  ScissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    D3D11_VIEWPORT              Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    ID3D11RasterizerState*      RS;
    ID3D11BlendState*           BlendState;
    FLOAT                       BlendFactor[4];
    UINT                        SampleMask;
    UINT                        StencilRef;
    ID3D11DepthStencilState*    DepthStencilState;
    ID3D11ShaderResourceView*   PSShaderResource;
    ID3D11SamplerState*         PSSampler;
    ID3D11PixelShader*          PS;
    ID3D11VertexShader*         VS;
    ID3D11GeometryShader*       GS;
    UINT                        PSInstancesCount, VSInstancesCount, GSInstancesCount;
    ID3D11ClassInstance         *PSInstances[256], *VSInstances[256], *GSInstances[256];   // 256 is max according to PSSetShader documentation
    D3D11_PRIMITIVE_TOPOLOGY    PrimitiveTopology;
    ID3D11Buffer*               IndexBuffer, *VertexBuffer, *VSConstantBuffer;
    UINT                        IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
    DXGI_FORMAT                 IndexBufferFormat;
    ID3D11InputLayout*          InputLayout;
  };
  BACKUP_DX11_STATE old;
  old.ScissorRectsCount = old.ViewportsCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
  ctx->RSGetScissorRects( &old.ScissorRectsCount, old.ScissorRects );
  ctx->RSGetViewports( &old.ViewportsCount, old.Viewports );
  ctx->RSGetState( &old.RS );
  ctx->OMGetBlendState( &old.BlendState, old.BlendFactor, &old.SampleMask );
  ctx->OMGetDepthStencilState( &old.DepthStencilState, &old.StencilRef );
  ctx->PSGetShaderResources( 0, 1, &old.PSShaderResource );
  ctx->PSGetSamplers( 0, 1, &old.PSSampler );
  old.PSInstancesCount = old.VSInstancesCount = old.GSInstancesCount = 256;
  ctx->PSGetShader( &old.PS, old.PSInstances, &old.PSInstancesCount );
  ctx->VSGetShader( &old.VS, old.VSInstances, &old.VSInstancesCount );
  ctx->VSGetConstantBuffers( 0, 1, &old.VSConstantBuffer );
  ctx->GSGetShader( &old.GS, old.GSInstances, &old.GSInstancesCount );

  ctx->IAGetPrimitiveTopology( &old.PrimitiveTopology );
  ctx->IAGetIndexBuffer( &old.IndexBuffer, &old.IndexBufferFormat, &old.IndexBufferOffset );
  ctx->IAGetVertexBuffers( 0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset );
  ctx->IAGetInputLayout( &old.InputLayout );

  // Setup desired DX state
  dx11_SetupRenderState( draw_data, ctx );

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
          dx11_SetupRenderState( draw_data, ctx );
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

  // Restore modified DX state
  ctx->RSSetScissorRects( old.ScissorRectsCount, old.ScissorRects );
  ctx->RSSetViewports( old.ViewportsCount, old.Viewports );
  ctx->RSSetState( old.RS ); if ( old.RS ) old.RS->Release();
  ctx->OMSetBlendState( old.BlendState, old.BlendFactor, old.SampleMask ); if ( old.BlendState ) old.BlendState->Release();
  ctx->OMSetDepthStencilState( old.DepthStencilState, old.StencilRef ); if ( old.DepthStencilState ) old.DepthStencilState->Release();
  ctx->PSSetShaderResources( 0, 1, &old.PSShaderResource ); if ( old.PSShaderResource ) old.PSShaderResource->Release();
  ctx->PSSetSamplers( 0, 1, &old.PSSampler ); if ( old.PSSampler ) old.PSSampler->Release();
  ctx->PSSetShader( old.PS, old.PSInstances, old.PSInstancesCount ); if ( old.PS ) old.PS->Release();
  for ( UINT i = 0; i < old.PSInstancesCount; i++ ) if ( old.PSInstances[i] ) old.PSInstances[i]->Release();
  ctx->VSSetShader( old.VS, old.VSInstances, old.VSInstancesCount ); if ( old.VS ) old.VS->Release();
  ctx->VSSetConstantBuffers( 0, 1, &old.VSConstantBuffer ); if ( old.VSConstantBuffer ) old.VSConstantBuffer->Release();
  ctx->GSSetShader( old.GS, old.GSInstances, old.GSInstancesCount ); if ( old.GS ) old.GS->Release();
  for ( UINT i = 0; i < old.VSInstancesCount; i++ ) if ( old.VSInstances[i] ) old.VSInstances[i]->Release();
  //ctx->IASetPrimitiveTopology( old.PrimitiveTopology );
  ctx->IASetIndexBuffer( old.IndexBuffer, old.IndexBufferFormat, old.IndexBufferOffset ); if ( old.IndexBuffer ) old.IndexBuffer->Release();
  ctx->IASetVertexBuffers( 0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset ); if ( old.VertexBuffer ) old.VertexBuffer->Release();
  ctx->IASetInputLayout( old.InputLayout ); if ( old.InputLayout ) old.InputLayout->Release();
}

void WinImgui::dx11_SetupRenderState( ImDrawData* draw_data, ID3D11DeviceContext* ctx )
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


bool WinImgui::win32_UpdateMouseCursor()
{
  ImGuiIO& io = ImGui::GetIO();
  if ( io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange )
    return false;

  ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
  if ( imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor )
  {
    // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
    ::SetCursor( NULL );
  }
  else
  {
    // Show OS mouse cursor
    LPTSTR win32_cursor = IDC_ARROW;
    switch ( imgui_cursor )
    {
    case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
    case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
    case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
    case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
    case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
    case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
    case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
    case ImGuiMouseCursor_Hand:         win32_cursor = IDC_HAND; break;
    }
    ::SetCursor( ::LoadCursor( NULL, win32_cursor ) );
  }
  return true;
}

void WinImgui::dx11_InvalidateDeviceObjects()
{
  if ( !md3dDevice )
    return;

  if ( mFontSampler ) { mFontSampler.Reset(); }
  if ( mFontTextureView ) { mFontTextureView.Reset(); ImGui::GetIO().Fonts->TexID = NULL; } // We copied mFontTextureView to io.Fonts->TexID so let's clear that as well.
  if ( mIB ) { mIB.Reset(); }
  if ( mVB ) { mVB.Reset(); }

  if ( mBlendState ) { mBlendState.Reset(); }
  if ( mDepthStencilState ) { mDepthStencilState.Reset(); }
  if ( mRasterizerState ) { mRasterizerState.Reset(); }
  if ( mPixelShader ) { mPixelShader.Reset(); }
  if ( mVertexConstantBuffer ) { mVertexConstantBuffer.Reset(); }
  if ( mInputLayout ) { mInputLayout.Reset(); }
  if ( mVertexShader ) { mVertexShader.Reset(); }
}

void WinImgui::dx11_CreateFontsTexture()
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


void WinImgui::win32_UpdateMousePos()
{
  ImGuiIO& io = ImGui::GetIO();

  // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
  if ( io.WantSetMousePos )
  {
    POINT pos ={ (int)io.MousePos.x, (int)io.MousePos.y };
    ::ClientToScreen( mhWnd, &pos );
    ::SetCursorPos( pos.x, pos.y );
  }

  // Set mouse position
  io.MousePos = ImVec2( -FLT_MAX, -FLT_MAX );
  POINT pos;
  if ( ::GetCursorPos( &pos ) && ::ScreenToClient( mhWnd, &pos ) )
    io.MousePos = ImVec2( (float)pos.x, (float)pos.y );
}


// Process Win32 mouse/keyboard inputs.
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
// PS: In this Win32 handler, we use the capture API (GetCapture/SetCapture/ReleaseCapture) to be able to read mouse coordinates when dragging mouse outside of our window bounds.
// PS: We treat DBLCLK messages as regular mouse down messages, so this code will work on windows classes that have the CS_DBLCLKS flag set. Our own example app code doesn't set this flag.
int WinImgui::win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  if ( ImGui::GetCurrentContext() == NULL )
    return 0;

  ImGuiIO& io = ImGui::GetIO();
  switch ( msg )
  {
  case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
  case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
  case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
  case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
  {
    int button = 0;
    if ( msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK ) { button = 0; }
    if ( msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK ) { button = 1; }
    if ( msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK ) { button = 2; }
    if ( msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK ) { button = ( GET_XBUTTON_WPARAM( wParam ) == XBUTTON1 ) ? 3 : 4; }
    if ( !ImGui::IsAnyMouseDown() && ::GetCapture() == NULL )
      ::SetCapture( hWnd );
    io.MouseDown[button] = true;
    return 0;
  }
  case WM_LBUTTONUP:
  case WM_RBUTTONUP:
  case WM_MBUTTONUP:
  case WM_XBUTTONUP:
  {
    int button = 0;
    if ( msg == WM_LBUTTONUP ) { button = 0; }
    if ( msg == WM_RBUTTONUP ) { button = 1; }
    if ( msg == WM_MBUTTONUP ) { button = 2; }
    if ( msg == WM_XBUTTONUP ) { button = ( GET_XBUTTON_WPARAM( wParam ) == XBUTTON1 ) ? 3 : 4; }
    io.MouseDown[button] = false;
    if ( !ImGui::IsAnyMouseDown() && ::GetCapture() == hWnd )
      ::ReleaseCapture();
    return 0;
  }
  case WM_MOUSEWHEEL:
    io.MouseWheel += (float)GET_WHEEL_DELTA_WPARAM( wParam ) / (float)WHEEL_DELTA;
    return 0;
  case WM_MOUSEHWHEEL:
    io.MouseWheelH += (float)GET_WHEEL_DELTA_WPARAM( wParam ) / (float)WHEEL_DELTA;
    return 0;
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
    if ( wParam < 256 )
      io.KeysDown[wParam] = 1;
    return 0;
  case WM_KEYUP:
  case WM_SYSKEYUP:
    if ( wParam < 256 )
      io.KeysDown[wParam] = 0;
    return 0;
  case WM_CHAR:
    // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
    io.AddInputCharacter( (unsigned int)wParam );
    return 0;
  case WM_SETCURSOR:
    if ( LOWORD( lParam ) == HTCLIENT && win32_UpdateMouseCursor() )
      return 1;
    return 0;
  }
  return 0;
}

#include "pch.hpp"
#include "imgui.h"
#include "WinImgui.hpp"

WinImgui::WinImgui( HWND hWnd, std::filesystem::path const& iniPath ) : mhWnd{ hWnd }, mTime{}, mTicksPerSecond{}, mLastMouseCursor{ ImGuiMouseCursor_COUNT }, mIniFilePath{ ( iniPath / "imgui.ini" ).string() }
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = mIniFilePath.c_str();

  if ( !::QueryPerformanceFrequency( (LARGE_INTEGER *)&mTicksPerSecond ) )
    throw std::exception{};
  if ( !::QueryPerformanceCounter( (LARGE_INTEGER *)&mTime ) )
    throw std::exception{};

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
  ImGui::DestroyContext();
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

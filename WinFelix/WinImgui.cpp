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

  io.BackendRendererName = "imgui_impl_dx11";
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

  ImGui::StyleColorsLight();
}

WinImgui::~WinImgui()
{
  ImGui::DestroyContext();
}


void WinImgui::newFrame()
{
  renderNewFrame();

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

static bool IsVkDown( int vk )
{
  return (::GetKeyState( vk ) & 0x8000) != 0;
}

static void ImGui_ImplWin32_AddKeyEvent( ImGuiKey key, bool down, int native_keycode, int native_scancode = -1 )
{
  ImGuiIO& io = ImGui::GetIO();
  io.AddKeyEvent( key, down );
  io.SetKeyEventNativeData( key, native_keycode, native_scancode ); // To support legacy indexing (<1.87 user code)
  IM_UNUSED( native_scancode );
}

static ImGuiKey ImGui_ImplWin32_VirtualKeyToImGuiKey( WPARAM wParam )
{
  switch (wParam)
  {
  case VK_TAB: return ImGuiKey_Tab;
  case VK_LEFT: return ImGuiKey_LeftArrow;
  case VK_RIGHT: return ImGuiKey_RightArrow;
  case VK_UP: return ImGuiKey_UpArrow;
  case VK_DOWN: return ImGuiKey_DownArrow;
  case VK_PRIOR: return ImGuiKey_PageUp;
  case VK_NEXT: return ImGuiKey_PageDown;
  case VK_HOME: return ImGuiKey_Home;
  case VK_END: return ImGuiKey_End;
  case VK_INSERT: return ImGuiKey_Insert;
  case VK_DELETE: return ImGuiKey_Delete;
  case VK_BACK: return ImGuiKey_Backspace;
  case VK_SPACE: return ImGuiKey_Space;
  case VK_RETURN: return ImGuiKey_Enter;
  case VK_ESCAPE: return ImGuiKey_Escape;
  case VK_OEM_7: return ImGuiKey_Apostrophe;
  case VK_OEM_COMMA: return ImGuiKey_Comma;
  case VK_OEM_MINUS: return ImGuiKey_Minus;
  case VK_OEM_PERIOD: return ImGuiKey_Period;
  case VK_OEM_2: return ImGuiKey_Slash;
  case VK_OEM_1: return ImGuiKey_Semicolon;
  case VK_OEM_PLUS: return ImGuiKey_Equal;
  case VK_OEM_4: return ImGuiKey_LeftBracket;
  case VK_OEM_5: return ImGuiKey_Backslash;
  case VK_OEM_6: return ImGuiKey_RightBracket;
  case VK_OEM_3: return ImGuiKey_GraveAccent;
  case VK_CAPITAL: return ImGuiKey_CapsLock;
  case VK_SCROLL: return ImGuiKey_ScrollLock;
  case VK_NUMLOCK: return ImGuiKey_NumLock;
  case VK_SNAPSHOT: return ImGuiKey_PrintScreen;
  case VK_PAUSE: return ImGuiKey_Pause;
  case VK_NUMPAD0: return ImGuiKey_Keypad0;
  case VK_NUMPAD1: return ImGuiKey_Keypad1;
  case VK_NUMPAD2: return ImGuiKey_Keypad2;
  case VK_NUMPAD3: return ImGuiKey_Keypad3;
  case VK_NUMPAD4: return ImGuiKey_Keypad4;
  case VK_NUMPAD5: return ImGuiKey_Keypad5;
  case VK_NUMPAD6: return ImGuiKey_Keypad6;
  case VK_NUMPAD7: return ImGuiKey_Keypad7;
  case VK_NUMPAD8: return ImGuiKey_Keypad8;
  case VK_NUMPAD9: return ImGuiKey_Keypad9;
  case VK_DECIMAL: return ImGuiKey_KeypadDecimal;
  case VK_DIVIDE: return ImGuiKey_KeypadDivide;
  case VK_MULTIPLY: return ImGuiKey_KeypadMultiply;
  case VK_SUBTRACT: return ImGuiKey_KeypadSubtract;
  case VK_ADD: return ImGuiKey_KeypadAdd;
  case VK_LSHIFT: return ImGuiKey_LeftShift;
  case VK_LCONTROL: return ImGuiKey_LeftCtrl;
  case VK_LMENU: return ImGuiKey_LeftAlt;
  case VK_LWIN: return ImGuiKey_LeftSuper;
  case VK_RSHIFT: return ImGuiKey_RightShift;
  case VK_RCONTROL: return ImGuiKey_RightCtrl;
  case VK_RMENU: return ImGuiKey_RightAlt;
  case VK_RWIN: return ImGuiKey_RightSuper;
  case VK_APPS: return ImGuiKey_Menu;
  case '0': return ImGuiKey_0;
  case '1': return ImGuiKey_1;
  case '2': return ImGuiKey_2;
  case '3': return ImGuiKey_3;
  case '4': return ImGuiKey_4;
  case '5': return ImGuiKey_5;
  case '6': return ImGuiKey_6;
  case '7': return ImGuiKey_7;
  case '8': return ImGuiKey_8;
  case '9': return ImGuiKey_9;
  case 'A': return ImGuiKey_A;
  case 'B': return ImGuiKey_B;
  case 'C': return ImGuiKey_C;
  case 'D': return ImGuiKey_D;
  case 'E': return ImGuiKey_E;
  case 'F': return ImGuiKey_F;
  case 'G': return ImGuiKey_G;
  case 'H': return ImGuiKey_H;
  case 'I': return ImGuiKey_I;
  case 'J': return ImGuiKey_J;
  case 'K': return ImGuiKey_K;
  case 'L': return ImGuiKey_L;
  case 'M': return ImGuiKey_M;
  case 'N': return ImGuiKey_N;
  case 'O': return ImGuiKey_O;
  case 'P': return ImGuiKey_P;
  case 'Q': return ImGuiKey_Q;
  case 'R': return ImGuiKey_R;
  case 'S': return ImGuiKey_S;
  case 'T': return ImGuiKey_T;
  case 'U': return ImGuiKey_U;
  case 'V': return ImGuiKey_V;
  case 'W': return ImGuiKey_W;
  case 'X': return ImGuiKey_X;
  case 'Y': return ImGuiKey_Y;
  case 'Z': return ImGuiKey_Z;
  case VK_F1: return ImGuiKey_F1;
  case VK_F2: return ImGuiKey_F2;
  case VK_F3: return ImGuiKey_F3;
  case VK_F4: return ImGuiKey_F4;
  case VK_F5: return ImGuiKey_F5;
  case VK_F6: return ImGuiKey_F6;
  case VK_F7: return ImGuiKey_F7;
  case VK_F8: return ImGuiKey_F8;
  case VK_F9: return ImGuiKey_F9;
  case VK_F10: return ImGuiKey_F10;
  case VK_F11: return ImGuiKey_F11;
  case VK_F12: return ImGuiKey_F12;
  default: return ImGuiKey_None;
  }
}

static void ImGui_ImplWin32_UpdateKeyModifiers()
{
  ImGuiIO& io = ImGui::GetIO();
  io.AddKeyEvent( ImGuiMod_Ctrl, IsVkDown( VK_CONTROL ) );
  io.AddKeyEvent( ImGuiMod_Shift, IsVkDown( VK_SHIFT ) );
  io.AddKeyEvent( ImGuiMod_Alt, IsVkDown( VK_MENU ) );
  io.AddKeyEvent( ImGuiMod_Super, IsVkDown( VK_APPS ) );
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
  case WM_KEYUP:
  case WM_SYSKEYDOWN:
  case WM_SYSKEYUP:
  {
    const bool is_key_down = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
    if (wParam < 256)
    {
      // Submit modifiers
      ImGui_ImplWin32_UpdateKeyModifiers();

      int vk = (int)wParam;

      // Submit key event
      const ImGuiKey key = ImGui_ImplWin32_VirtualKeyToImGuiKey( vk );
      const int scancode = (int)LOBYTE( HIWORD( lParam ) );
      if (key != ImGuiKey_None)
        ImGui_ImplWin32_AddKeyEvent( key, is_key_down, vk, scancode );

      // Submit individual left/right modifier events
      if (vk == VK_SHIFT)
      {
        // Important: Shift keys tend to get stuck when pressed together, missing key-up events are corrected in ImGui_ImplWin32_ProcessKeyEventsWorkarounds()
        if (IsVkDown( VK_LSHIFT ) == is_key_down) { ImGui_ImplWin32_AddKeyEvent( ImGuiKey_LeftShift, is_key_down, VK_LSHIFT, scancode ); }
        if (IsVkDown( VK_RSHIFT ) == is_key_down) { ImGui_ImplWin32_AddKeyEvent( ImGuiKey_RightShift, is_key_down, VK_RSHIFT, scancode ); }
      }
      else if (vk == VK_CONTROL)
      {
        if (IsVkDown( VK_LCONTROL ) == is_key_down) { ImGui_ImplWin32_AddKeyEvent( ImGuiKey_LeftCtrl, is_key_down, VK_LCONTROL, scancode ); }
        if (IsVkDown( VK_RCONTROL ) == is_key_down) { ImGui_ImplWin32_AddKeyEvent( ImGuiKey_RightCtrl, is_key_down, VK_RCONTROL, scancode ); }
      }
      else if (vk == VK_MENU)
      {
        if (IsVkDown( VK_LMENU ) == is_key_down) { ImGui_ImplWin32_AddKeyEvent( ImGuiKey_LeftAlt, is_key_down, VK_LMENU, scancode ); }
        if (IsVkDown( VK_RMENU ) == is_key_down) { ImGui_ImplWin32_AddKeyEvent( ImGuiKey_RightAlt, is_key_down, VK_RMENU, scancode ); }
      }
    }
    return 0;
  }
  case WM_SETFOCUS:
  case WM_KILLFOCUS:
    io.AddFocusEvent( msg == WM_SETFOCUS );
    return 0;
  case WM_CHAR:
    if (::IsWindowUnicode( hWnd ))
    {
      if (wParam > 0 && wParam < 0x10000)
        io.AddInputCharacterUTF16( (unsigned short)wParam );
    }
    else
    {
      wchar_t wch = 0;
      ::MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, (char*)&wParam, 1, &wch, 1 );
      io.AddInputCharacter( wch );
    }
    return 0;
  case WM_SETCURSOR:
    if ( LOWORD( lParam ) == HTCLIENT && win32_UpdateMouseCursor() )
      return 1;
    return 0;
  }
  return 0;
}

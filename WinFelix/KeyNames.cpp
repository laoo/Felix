#include "pch.hpp"
#include "KeyNames.hpp"

namespace
{

class KeyNames
{
public:
  KeyNames();
  char const* name( uint32_t key ) const;
private:
  std::array<char const*, 256> mNames;
};

KeyNames::KeyNames() : mNames{}
{
  mNames[VK_LBUTTON] = "Left Button";
  mNames[VK_RBUTTON] = "Right Button";
  mNames[VK_CANCEL] = "Break";
  mNames[VK_MBUTTON] = "Middle Button";
  mNames[VK_XBUTTON1] = "X Button 1";
  mNames[VK_XBUTTON2] = "X Button 2";
  mNames[VK_BACK] = "Backspace";
  mNames[VK_TAB] = "Tab";
  mNames[VK_CLEAR] = "Clear";
  mNames[VK_RETURN] = "Enter";
  mNames[VK_SHIFT] = "Shift";
  mNames[VK_CONTROL] = "Ctrl";
  mNames[VK_MENU] = "Alt";
  mNames[VK_PAUSE] = "Pause";
  mNames[VK_CAPITAL] = "Caps Lock";
  mNames[VK_KANA] = "Kana";
  mNames[VK_JUNJA] = "Junja";
  mNames[VK_FINAL] = "Final";
  mNames[VK_KANJI] = "Kanji";
  mNames[VK_ESCAPE] = "Esc";
  mNames[VK_CONVERT] = "Convert";
  mNames[VK_NONCONVERT] = "Non Convert";
  mNames[VK_ACCEPT] = "Accept";
  mNames[VK_MODECHANGE] = "Mode Change";
  mNames[VK_SPACE] = "Space";
  mNames[VK_PRIOR] = "Page Up";
  mNames[VK_NEXT] = "Page Down";
  mNames[VK_END] = "End";
  mNames[VK_HOME] = "Home";
  mNames[VK_LEFT] = "Left";
  mNames[VK_UP] = "Up";
  mNames[VK_RIGHT] = "Right";
  mNames[VK_DOWN] = "Down";
  mNames[VK_SELECT] = "Select";
  mNames[VK_PRINT] = "Print";
  mNames[VK_EXECUTE] = "Execute";
  mNames[VK_SNAPSHOT] = "Print Screen";
  mNames[VK_INSERT] = "Insert";
  mNames[VK_DELETE] = "Delete";
  mNames[VK_HELP] = "Help";
  mNames['0'] = "0";
  mNames['1'] = "1";
  mNames['2'] = "2";
  mNames['3'] = "3";
  mNames['4'] = "4";
  mNames['5'] = "5";
  mNames['6'] = "6";
  mNames['7'] = "7";
  mNames['8'] = "8";
  mNames['9'] = "9";
  mNames['A'] = "A";
  mNames['B'] = "B";
  mNames['C'] = "C";
  mNames['D'] = "D";
  mNames['E'] = "E";
  mNames['F'] = "F";
  mNames['G'] = "G";
  mNames['H'] = "H";
  mNames['I'] = "I";
  mNames['J'] = "J";
  mNames['K'] = "K";
  mNames['L'] = "L";
  mNames['M'] = "M";
  mNames['N'] = "N";
  mNames['O'] = "O";
  mNames['P'] = "P";
  mNames['Q'] = "Q";
  mNames['R'] = "R";
  mNames['S'] = "S";
  mNames['T'] = "T";
  mNames['U'] = "U";
  mNames['V'] = "V";
  mNames['W'] = "W";
  mNames['X'] = "X";
  mNames['Y'] = "Y";
  mNames['Z'] = "Z";
  mNames[VK_LWIN] = "Left Win";
  mNames[VK_RWIN] = "Right Win";
  mNames[VK_APPS] = "Applications";
  mNames[VK_SLEEP] = "Sleep";
  mNames[VK_NUMPAD0] = "Numpad 0";
  mNames[VK_NUMPAD1] = "Numpad 1";
  mNames[VK_NUMPAD2] = "Numpad 2";
  mNames[VK_NUMPAD3] = "Numpad 3";
  mNames[VK_NUMPAD4] = "Numpad 4";
  mNames[VK_NUMPAD5] = "Numpad 5";
  mNames[VK_NUMPAD6] = "Numpad 6";
  mNames[VK_NUMPAD7] = "Numpad 7";
  mNames[VK_NUMPAD8] = "Numpad 8";
  mNames[VK_NUMPAD9] = "Numpad 9";
  mNames[VK_MULTIPLY] = "Numpad *";
  mNames[VK_ADD] = "Numpad +";
  mNames[VK_SEPARATOR] = "Separator";
  mNames[VK_SUBTRACT] = "Num -";
  mNames[VK_DECIMAL] = "Numpad .";
  mNames[VK_DIVIDE] = "Numpad /";
  mNames[VK_F1] = "F1";
  mNames[VK_F2] = "F2";
  mNames[VK_F3] = "F3";
  mNames[VK_F4] = "F4";
  mNames[VK_F5] = "F5";
  mNames[VK_F6] = "F6";
  mNames[VK_F7] = "F7";
  mNames[VK_F8] = "F8";
  mNames[VK_F9] = "F9";
  mNames[VK_F10] = "F10";
  mNames[VK_F11] = "F11";
  mNames[VK_F12] = "F12";
  mNames[VK_F13] = "F13";
  mNames[VK_F14] = "F14";
  mNames[VK_F15] = "F15";
  mNames[VK_F16] = "F16";
  mNames[VK_F17] = "F17";
  mNames[VK_F18] = "F18";
  mNames[VK_F19] = "F19";
  mNames[VK_F20] = "F20";
  mNames[VK_F21] = "F21";
  mNames[VK_F22] = "F22";
  mNames[VK_F23] = "F23";
  mNames[VK_F24] = "F24";
  mNames[VK_NUMLOCK] = "Num Lock";
  mNames[VK_SCROLL] = "Scrol Lock";
  mNames[VK_LSHIFT] = "Left Shift";
  mNames[VK_RSHIFT] = "Right Shift";
  mNames[VK_LCONTROL] = "Left Ctrl";
  mNames[VK_RCONTROL] = "Right Ctrl";
  mNames[VK_LMENU] = "Left Alt";
  mNames[VK_RMENU] = "Right Alt";
  mNames[VK_BROWSER_BACK] = "Browser Back";
  mNames[VK_BROWSER_FORWARD] = "Browser Forward";
  mNames[VK_BROWSER_REFRESH] = "Browser Refresh";
  mNames[VK_BROWSER_STOP] = "Browser Stop";
  mNames[VK_BROWSER_SEARCH] = "Browser Search";
  mNames[VK_BROWSER_FAVORITES] = "Browser Favorites";
  mNames[VK_BROWSER_HOME] = "Browser Home";
  mNames[VK_VOLUME_MUTE] = "Volume Mute";
  mNames[VK_VOLUME_DOWN] = "Volume Down";
  mNames[VK_VOLUME_UP] = "Volume Up";
  mNames[VK_MEDIA_NEXT_TRACK] = "Next Track";
  mNames[VK_MEDIA_PREV_TRACK] = "Previous Track";
  mNames[VK_MEDIA_STOP] = "Stop";
  mNames[VK_MEDIA_PLAY_PAUSE] = "Play / Pause";
  mNames[VK_LAUNCH_MAIL] = "Mail";
  mNames[VK_LAUNCH_MEDIA_SELECT] = "Media";
  mNames[VK_LAUNCH_APP1] = "App1";
  mNames[VK_LAUNCH_APP2] = "App2";
  mNames[VK_OEM_1] = ";:";
  mNames[VK_OEM_PLUS] = "=+";
  mNames[VK_OEM_COMMA] = ",<";
  mNames[VK_OEM_MINUS] = "-_";
  mNames[VK_OEM_PERIOD] = ".>";
  mNames[VK_OEM_2] = "/?";
  mNames[VK_OEM_3] = "~`";
  mNames[VK_GAMEPAD_A] = "Gamepad A";
  mNames[VK_GAMEPAD_B] = "Gamepad B";
  mNames[VK_GAMEPAD_X] = "Gamepad X";
  mNames[VK_GAMEPAD_Y] = "Gamepad Y";
  mNames[VK_GAMEPAD_RIGHT_SHOULDER] = "Gamepad Right Shoulder";
  mNames[VK_GAMEPAD_LEFT_SHOULDER] = "Gamepad Left Shoulder";
  mNames[VK_GAMEPAD_LEFT_TRIGGER] = "Gamepad Left Trigger";
  mNames[VK_GAMEPAD_RIGHT_TRIGGER] = "Gamepad Right Trigger";
  mNames[VK_GAMEPAD_DPAD_UP] = "Gamepad DPad Up";
  mNames[VK_GAMEPAD_DPAD_DOWN] = "Gamepad DPad Down";
  mNames[VK_GAMEPAD_DPAD_LEFT] = "Gamepad DPad Left";
  mNames[VK_GAMEPAD_DPAD_RIGHT] = "Gamepad DPad Right";
  mNames[VK_GAMEPAD_MENU] = "Gamepad Menu";
  mNames[VK_GAMEPAD_VIEW] = "Gamepad View";
  mNames[VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON] = "Gamepad Left Thumbstick Button";
  mNames[VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON] = "Gamepad Right Thumbstick Button";
  mNames[VK_GAMEPAD_LEFT_THUMBSTICK_UP] = "Gamepad Left Thumbstick Up";
  mNames[VK_GAMEPAD_LEFT_THUMBSTICK_DOWN] = "Gamepad Left Thumbstick Down";
  mNames[VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT] = "Gamepad Left Thumbstick Right";
  mNames[VK_GAMEPAD_LEFT_THUMBSTICK_LEFT] = "Gamepad Left Thumbstick Left";
  mNames[VK_GAMEPAD_RIGHT_THUMBSTICK_UP] = "Gamepad Right Thumbstick Up";
  mNames[VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN] = "Gamepad Right Thumbstick Down";
  mNames[VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT] = "Gamepad Right Thumbstick Right";
  mNames[VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT] = "Gamepad Right Thumbstick Left";
  mNames[VK_OEM_4] = "[{";
  mNames[VK_OEM_5] = "\\|";
  mNames[VK_OEM_6] = "]}";
  mNames[VK_OEM_7] = "'\"";
  mNames[VK_OEM_8] = "§!";
  mNames[VK_OEM_AX] = "AX";
  mNames[VK_OEM_102] = "<>";
  mNames[VK_ICO_HELP] = "ICO Help";
  mNames[VK_ICO_00] = "ICO 00";
  mNames[VK_PROCESSKEY] = "Process";
  mNames[VK_ICO_CLEAR] = "ICO Clear";
  mNames[VK_PACKET] = "Packet";
  mNames[VK_ATTN] = "Attn";
  mNames[VK_CRSEL] = "Cr Sel";
  mNames[VK_EXSEL] = "Ex Sel";
  mNames[VK_EREOF] = "Er Eof";
  mNames[VK_PLAY] = "Play";
  mNames[VK_ZOOM] = "Zoom";
  mNames[VK_NONAME] = "NoName";
  mNames[VK_PA1] = "Pa1";
  mNames[VK_OEM_CLEAR] = "OemClr";
}

char const* KeyNames::name( uint32_t key ) const
{
  return key < mNames.size() ? mNames[key] : nullptr;
}

}

char const* keyName( uint32_t key )
{
  static KeyNames keyNames;

  return keyNames.name( key );
}

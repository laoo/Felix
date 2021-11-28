#include "pch.hpp"
#include "UserInput.hpp"
#include "SysConfig.hpp"

UserInput::UserInput( SysConfig const& cfg ) : mXInputDLL{}, mXInputGetCapabilities{}, mXInputGetState{}, mRotation{ ImageProperties::Rotation::NORMAL }, mMapping{}, mPressedCodes{}, mLastState{}, mGamepadPacket{}, mHasGamepad{}
{
  const char* xinput_dll_names[] =
  {
      "xinput1_4.dll",   // Windows 8+
      "xinput1_3.dll",   // DirectX SDK
      "xinput9_1_0.dll", // Windows Vista, Windows 7
      "xinput1_2.dll",   // DirectX SDK
      "xinput1_1.dll"    // DirectX SDK
  };

  for ( auto ptr : xinput_dll_names )
  {
    if ( HMODULE dll = ::LoadLibraryA( ptr ) )
    {
      mXInputDLL = dll;
      mXInputGetCapabilities = ( PFN_XInputGetCapabilities )::GetProcAddress( dll, "XInputGetCapabilities" );
      mXInputGetState = ( PFN_XInputGetState )::GetProcAddress( dll, "XInputGetState" );
      break;
    }
  }

  mMapping[KeyInput::OUTER] = cfg.keyMapping.outer;
  mMapping[KeyInput::INNER] = cfg.keyMapping.inner;
  mMapping[KeyInput::OPTION2] = cfg.keyMapping.option2;
  mMapping[KeyInput::OPTION1] = cfg.keyMapping.option1;
  mMapping[KeyInput::RIGHT] = cfg.keyMapping.right;
  mMapping[KeyInput::LEFT] = cfg.keyMapping.left;
  mMapping[KeyInput::DOWN] = cfg.keyMapping.down;
  mMapping[KeyInput::UP] = cfg.keyMapping.up;
  mMapping[KeyInput::PAUSE] = cfg.keyMapping.pause;

  recheckGamepad();
}

UserInput::~UserInput()
{
  if ( mXInputDLL )
    ::FreeLibrary( mXInputDLL );
}

void UserInput::serialize( SysConfig& cfg )
{
  cfg.keyMapping.outer = mMapping[KeyInput::OUTER];
  cfg.keyMapping.inner = mMapping[KeyInput::INNER];
  cfg.keyMapping.option2 = mMapping[KeyInput::OPTION2];
  cfg.keyMapping.option1 = mMapping[KeyInput::OPTION1];
  cfg.keyMapping.right = mMapping[KeyInput::RIGHT];
  cfg.keyMapping.left = mMapping[KeyInput::LEFT];
  cfg.keyMapping.down = mMapping[KeyInput::DOWN];
  cfg.keyMapping.up = mMapping[KeyInput::UP];
  cfg.keyMapping.pause = mMapping[KeyInput::PAUSE];
}

void UserInput::keyDown( int code )
{
  std::unique_lock<std::mutex> l{ mMutex };

  switch ( code )
  {
  case VK_SHIFT:
    if ( ( GetAsyncKeyState( VK_LSHIFT ) & 0x8000 ) != 0 )
      code = VK_LSHIFT;
    if ( ( GetAsyncKeyState( VK_RSHIFT ) & 0x8000 ) != 0 )
      code = VK_RSHIFT;
    break;
  case VK_CONTROL:
    if ( ( GetAsyncKeyState( VK_LCONTROL ) & 0x8000 ) != 0 )
      code = VK_LCONTROL;
    if ( ( GetAsyncKeyState( VK_RCONTROL ) & 0x8000 ) != 0 )
      code = VK_RCONTROL;
    break;
  case VK_MENU:
    if ( ( GetAsyncKeyState( VK_LMENU ) & 0x8000 ) != 0 )
      code = VK_LMENU;
    if ( ( GetAsyncKeyState( VK_RMENU ) & 0x8000 ) != 0 )
      code = VK_RMENU;
    break;
  default:
    break;
  }

  if ( !pressed( code ) )
    mPressedCodes.push_back( code );
}

void UserInput::keyUp( int code )
{
  std::unique_lock<std::mutex> l{ mMutex };

  switch ( code )
  {
  case VK_SHIFT:
    if ( ( GetAsyncKeyState( VK_LSHIFT ) & 0x8000 ) == 0 )
    {
      mPressedCodes.erase( std::remove( mPressedCodes.begin(), mPressedCodes.end(), VK_LSHIFT ), mPressedCodes.end() );
    }
    if ( ( GetAsyncKeyState( VK_RSHIFT ) & 0x8000 ) == 0 )
    {
      mPressedCodes.erase( std::remove( mPressedCodes.begin(), mPressedCodes.end(), VK_RSHIFT ), mPressedCodes.end() );
    }
    break;
  case VK_CONTROL:
    if ( ( GetAsyncKeyState( VK_LCONTROL ) & 0x8000 ) == 0 )
    {
      mPressedCodes.erase( std::remove( mPressedCodes.begin(), mPressedCodes.end(), VK_LCONTROL ), mPressedCodes.end() );
    }
    if ( ( GetAsyncKeyState( VK_RCONTROL ) & 0x8000 ) == 0 )
    {
      mPressedCodes.erase( std::remove( mPressedCodes.begin(), mPressedCodes.end(), VK_RCONTROL ), mPressedCodes.end() );
    }
    break;
  case VK_MENU:
    if ( ( GetAsyncKeyState( VK_LMENU ) & 0x8000 ) == 0 )
    {
      mPressedCodes.erase( std::remove( mPressedCodes.begin(), mPressedCodes.end(), VK_LMENU ), mPressedCodes.end() );
    }
    if ( ( GetAsyncKeyState( VK_RMENU ) & 0x8000 ) == 0 )
    {
      mPressedCodes.erase( std::remove( mPressedCodes.begin(), mPressedCodes.end(), VK_RMENU ), mPressedCodes.end() );
    }
    break;
  default:
    mPressedCodes.erase( std::remove( mPressedCodes.begin(), mPressedCodes.end(), code ), mPressedCodes.end() );
    return;
  }
}

void UserInput::lostFocus()
{
  std::unique_lock<std::mutex> l{ mMutex };

  mPressedCodes.clear();
}

void UserInput::setRotation( ImageProperties::Rotation rotation )
{
  mRotation = rotation;
}

void UserInput::updateGamepad()
{
  if ( !mXInputDLL || !mHasGamepad )
    return;

  XINPUT_STATE xInputState;
  auto result = mXInputGetState( 0, &xInputState );

  if ( mGamepadPacket == xInputState.dwPacketNumber || result != ERROR_SUCCESS )
    return;

  mGamepadPacket = xInputState.dwPacketNumber;
  mLastState = xInputState.Gamepad;
}

void UserInput::recheckGamepad()
{
  if ( !mXInputDLL )
    return;

  XINPUT_CAPABILITIES caps;
  auto result = mXInputGetCapabilities( 0, XINPUT_FLAG_GAMEPAD, &caps );
  mHasGamepad = result == ERROR_SUCCESS;
}

KeyInput UserInput::getInput( bool leftHand ) const
{
  std::unique_lock<std::mutex> l{ mMutex };

  bool outer = pressed( KeyInput::OUTER );
  bool inner = pressed( KeyInput::INNER );
  bool opt1 = pressed( KeyInput::OPTION1 );
  bool opt2 = pressed( KeyInput::OPTION2 );
  bool pause = pressed( KeyInput::PAUSE );
  bool left = pressed( leftHand ? KeyInput::RIGHT : KeyInput::LEFT );
  bool right = pressed( leftHand ? KeyInput::LEFT : KeyInput::RIGHT );
  bool up = pressed( leftHand ? KeyInput::DOWN : KeyInput::UP );
  bool down = pressed( leftHand ? KeyInput::UP : KeyInput::DOWN );

  if ( mHasGamepad )
  {
    right |= ( mLastState.sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ) || ( ( mLastState.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT ) != 0 );
    left |= ( mLastState.sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ) || ( ( mLastState.wButtons & XINPUT_GAMEPAD_DPAD_LEFT ) != 0 );
    up |= ( mLastState.sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ) || ( ( mLastState.wButtons & XINPUT_GAMEPAD_DPAD_UP ) != 0 );
    down |= ( mLastState.sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ) || ( ( mLastState.wButtons & XINPUT_GAMEPAD_DPAD_DOWN ) != 0 );
    outer |= ( ( mLastState.wButtons & XINPUT_GAMEPAD_B ) != 0 ) || ( ( mLastState.wButtons & XINPUT_GAMEPAD_Y ) != 0 );
    inner |= ( ( mLastState.wButtons & XINPUT_GAMEPAD_A ) != 0 ) || ( ( mLastState.wButtons & XINPUT_GAMEPAD_X ) != 0 );
    opt1 |= ( ( mLastState.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER ) != 0 );
    opt2 |= ( ( mLastState.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER ) != 0 );
    pause |= ( ( mLastState.wButtons & XINPUT_GAMEPAD_START ) != 0 );
  }

  KeyInput result{};

  result.set( KeyInput::OUTER, outer );
  result.set( KeyInput::INNER, inner );
  result.set( KeyInput::OPTION1, opt1 );
  result.set( KeyInput::OPTION2, opt2 );
  result.set( KeyInput::PAUSE, pause );

  switch ( mRotation )
  {
  case ImageProperties::Rotation::RIGHT:
    result.set( KeyInput::LEFT, down );
    result.set( KeyInput::RIGHT, up );
    result.set( KeyInput::UP, left );
    result.set( KeyInput::DOWN, right );
    break;
  case ImageProperties::Rotation::LEFT:
    result.set( KeyInput::LEFT, up );
    result.set( KeyInput::RIGHT, down );
    result.set( KeyInput::UP, right );
    result.set( KeyInput::DOWN, left );
    break;
  default:
    result.set( KeyInput::LEFT, left );
    result.set( KeyInput::RIGHT, right );
    result.set( KeyInput::UP, up );
    result.set( KeyInput::DOWN, down );
    break;
  }

  return result;
}

int UserInput::getVirtualCode( KeyInput::Key k )
{
  return mMapping[k];
}

void UserInput::updateMapping( KeyInput::Key k, int code )
{
  mMapping[k] = code;
}

int UserInput::firstKeyPressed() const
{
  std::unique_lock<std::mutex> l{ mMutex };

  if ( mPressedCodes.empty() )
    return 0;
  else
    return mPressedCodes[0];
  return 0;
}

bool UserInput::pressed( int code ) const
{
  return std::find( mPressedCodes.cbegin(), mPressedCodes.cend(), code ) != mPressedCodes.cend();
}

bool UserInput::pressed( KeyInput::Key key ) const
{
  return pressed( mMapping[key] );
}

#pragma once
#include "IInputSource.hpp"
#include "Rotation.hpp"

struct SysConfig;

class UserInput : public IInputSource
{
public:
  UserInput( SysConfig const& cfg );
  ~UserInput() override;

  void serialize( SysConfig& cfg );

  void keyDown( int code );
  void keyUp( int code );
  void lostFocus();
  void setRotation( Rotation rotation );

  KeyInput getInput( bool leftHand ) const override;

  int getVirtualCode( KeyInput::Key k );
  void updateMapping( KeyInput::Key k, int code );


  int firstKeyPressed() const;

  void updateGamepad();
  void recheckGamepad();

private:

  bool pressed( int code ) const;
  bool pressed( KeyInput::Key key ) const;


private:

private:
  typedef DWORD( WINAPI* PFN_XInputGetCapabilities )( DWORD, DWORD, XINPUT_CAPABILITIES* );
  typedef DWORD( WINAPI* PFN_XInputGetState )( DWORD, XINPUT_STATE* );

  HMODULE                     mXInputDLL;
  PFN_XInputGetCapabilities   mXInputGetCapabilities;
  PFN_XInputGetState          mXInputGetState;

  mutable std::mutex mMutex;
  Rotation mRotation;
  std::array<int, 9> mMapping;
  std::vector<int> mPressedCodes;
  XINPUT_GAMEPAD mLastState;
  uint32_t mGamepadPacket;
  bool mHasGamepad;

};

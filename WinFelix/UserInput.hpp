#pragma once
#include "IInputSource.hpp"
#include "Rotation.hpp"

struct SysConfig;

class UserInput : public IInputSource
{
public:
  UserInput( SysConfig const& cfg );
  ~UserInput() override;

  void keyDown( int code );
  void keyUp( int code );
  void lostFocus();
  void setRotation( Rotation rotation );

  KeyInput getInput( bool leftHand ) const override;

  int firstKeyPressed() const;

private:

  bool pressed( int code ) const;
  bool pressed( KeyInput::Key key ) const;


private:

private:
  mutable std::mutex mMutex;
  Rotation mRotation;
  std::array<int, 9> mMapping;
  std::vector<int> mPressedCodes;

};

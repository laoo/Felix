#include "pch.hpp"
#include "UserInput.hpp"
#include "SysConfig.hpp"

UserInput::UserInput( SysConfig const& cfg ) : mRotation{ Rotation::NORMAL }, mMapping{}, mPressedCodes{}
{
  mMapping[KeyInput::OUTER] = cfg.keyMapping.outer;
  mMapping[KeyInput::INNER] = cfg.keyMapping.inner;
  mMapping[KeyInput::OPTION2] = cfg.keyMapping.option2;
  mMapping[KeyInput::OPTION1] = cfg.keyMapping.option1;
  mMapping[KeyInput::RIGHT] = cfg.keyMapping.right;
  mMapping[KeyInput::LEFT] = cfg.keyMapping.left;
  mMapping[KeyInput::DOWN] = cfg.keyMapping.down;
  mMapping[KeyInput::UP] = cfg.keyMapping.up;
  mMapping[KeyInput::PAUSE] = cfg.keyMapping.pause;
}

UserInput::~UserInput()
{
}

void UserInput::keyDown( int code )
{
  std::unique_lock<std::mutex> l{ mMutex };

  if ( !pressed( code ) )
    mPressedCodes.push_back( code );
}

void UserInput::keyUp( int code )
{
  std::unique_lock<std::mutex> l{ mMutex };

  mPressedCodes.erase( std::remove( mPressedCodes.begin(), mPressedCodes.end(), code ), mPressedCodes.end() );
}

void UserInput::lostFocus()
{
  std::unique_lock<std::mutex> l{ mMutex };

  mPressedCodes.clear();
}

void UserInput::setRotation( Rotation rotation )
{
  mRotation = rotation;
}

KeyInput UserInput::getInput( bool leftHand ) const
{
  std::unique_lock<std::mutex> l{ mMutex };

  KeyInput result{};

  result.set( KeyInput::OUTER, pressed( KeyInput::OUTER ) );
  result.set( KeyInput::INNER, pressed( KeyInput::INNER ) );
  result.set( KeyInput::OPTION1, pressed( KeyInput::OPTION1 ) );
  result.set( KeyInput::OPTION2, pressed( KeyInput::OPTION2 ) );
  result.set( KeyInput::PAUSE, pressed( KeyInput::PAUSE ) );
  switch ( mRotation )
  {
  case Rotation::NORMAL:
    result.set( KeyInput::LEFT, pressed( leftHand ? KeyInput::RIGHT : KeyInput::LEFT ) );
    result.set( KeyInput::RIGHT, pressed( leftHand ? KeyInput::LEFT : KeyInput::RIGHT ) );
    result.set( KeyInput::UP, pressed( leftHand ? KeyInput::DOWN : KeyInput::UP ) );
    result.set( KeyInput::DOWN, pressed( leftHand ? KeyInput::UP : KeyInput::DOWN ) );
    break;
  case Rotation::LEFT:
    result.set( KeyInput::LEFT, pressed( leftHand ? KeyInput::UP : KeyInput::DOWN ) );
    result.set( KeyInput::RIGHT, pressed( leftHand ? KeyInput::DOWN : KeyInput::UP ) );
    result.set( KeyInput::UP, pressed( leftHand ? KeyInput::RIGHT : KeyInput::LEFT ) );
    result.set( KeyInput::DOWN, pressed( leftHand ? KeyInput::LEFT : KeyInput::RIGHT ) );
    break;
  case Rotation::RIGHT:
    result.set( KeyInput::LEFT, pressed( leftHand ? KeyInput::DOWN : KeyInput::UP ) );
    result.set( KeyInput::RIGHT, pressed( leftHand ? KeyInput::UP : KeyInput::DOWN ) );
    result.set( KeyInput::UP, pressed( leftHand ? KeyInput::LEFT : KeyInput::RIGHT ) );
    result.set( KeyInput::DOWN, pressed( leftHand ? KeyInput::RIGHT : KeyInput::LEFT ) );
    break;
  }

  return result;
}

bool UserInput::pressed( int code ) const
{
  return std::find( mPressedCodes.cbegin(), mPressedCodes.cend(), code ) != mPressedCodes.cend();
}

bool UserInput::pressed( KeyInput::Key key ) const
{
  return pressed( mMapping[key] );
}

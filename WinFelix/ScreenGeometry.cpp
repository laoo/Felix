#include "pch.hpp"
#include "ScreenGeometry.hpp"
#include "Utility.hpp"

ScreenGeometry::ScreenGeometry() : mWinWidth{}, mWinHeight{}, mScale{ 1 }, mRotation{}
{
}

bool ScreenGeometry::update( int width, int height, ImageProperties::Rotation rotation )
{
  if ( mWinHeight != height || mWinWidth != width || mRotation != rotation )
  {
    mWinHeight = height;
    mWinWidth = width;
    mRotation = rotation;

    int sx, sy;
    switch ( mRotation )
    {
    case ImageProperties::Rotation::LEFT:
    case ImageProperties::Rotation::RIGHT:
      sx = (std::max)( 1, mWinWidth / SCREEN_HEIGHT );
      sy = (std::max)( 1, mWinHeight / SCREEN_WIDTH );
      break;
    default:
      sx = (std::max)( 1, mWinWidth / SCREEN_WIDTH );
      sy = (std::max)( 1, mWinHeight / SCREEN_HEIGHT );
      break;
    }

    mScale = (std::min)( sx, sy );
    return true;
  }

  return false;
}

int ScreenGeometry::windowWidth() const
{
  return mWinWidth;
}

int ScreenGeometry::windowHeight() const
{
  return mWinHeight;
}

int ScreenGeometry::minWindowWidth() const
{
  return mRotation == ImageProperties::Rotation::NORMAL ? SCREEN_WIDTH : SCREEN_HEIGHT;
}

int ScreenGeometry::minWindowHeight() const
{
  return mRotation == ImageProperties::Rotation::NORMAL ? SCREEN_HEIGHT : SCREEN_WIDTH;
}

int ScreenGeometry::width() const
{
  return minWindowWidth() * mScale;
}

int ScreenGeometry::height() const
{
  return minWindowHeight() * mScale;
}

int ScreenGeometry::xOff() const
{
  switch ( mRotation )
  {
  case ImageProperties::Rotation::LEFT:
    return ( mWinWidth + SCREEN_HEIGHT * mScale ) / 2;
  case ImageProperties::Rotation::RIGHT:
    return ( mWinWidth - SCREEN_HEIGHT * mScale ) / 2;
  default:
    return ( mWinWidth - SCREEN_WIDTH * mScale ) / 2;
  }
}

int ScreenGeometry::yOff() const
{
  switch ( mRotation )
  {
  case ImageProperties::Rotation::LEFT:
    return ( mWinHeight - SCREEN_WIDTH * mScale ) / 2;
  case ImageProperties::Rotation::RIGHT:
    return ( mWinHeight + SCREEN_WIDTH * mScale ) / 2;
  default:
    return ( mWinHeight - SCREEN_HEIGHT * mScale ) / 2;
  }
}

int ScreenGeometry::scale() const
{
  return mScale;
}

int ScreenGeometry::rotx1() const
{
  switch ( mRotation )
  {
  case ImageProperties::Rotation::LEFT:
    return 0;  //mScale * cos( 90 )
  case ImageProperties::Rotation::RIGHT:
    return 0;  //mScale * cos( -90 )
  default:
    return mScale;  //mScale * cos( 0 )
  }
}

int ScreenGeometry::rotx2() const
{
  switch ( mRotation )
  {
  case ImageProperties::Rotation::LEFT:
    return -mScale;  //mScale * -sin( 90 )
  case ImageProperties::Rotation::RIGHT:
    return mScale;  //mScale * -sin( -90 )
  default:
    return 0;  //mScale * -sin( 0 )
  }
}

int ScreenGeometry::roty1() const
{
  switch ( mRotation )
  {
  case ImageProperties::Rotation::LEFT:
    return mScale;  //mScale * sin( 90 )
  case ImageProperties::Rotation::RIGHT:
    return -mScale;  //mScale * sin( -90 )
  default:
    return 0;  //mScale * sin( 0 )
  }
}

int ScreenGeometry::roty2() const
{
  switch ( mRotation )
  {
  case ImageProperties::Rotation::LEFT:
    return 0;  //mScale * cos( 90 )
  case ImageProperties::Rotation::RIGHT:
    return 0;  //mScale * cos( -90 )
  default:
    return mScale;  //mScale * cos( 0 )
  }
}

ImageProperties::Rotation ScreenGeometry::rotation() const
{
  return mRotation;
}

ScreenGeometry::operator bool() const
{
  return mWinWidth != 0 && mWinHeight != 0;
}


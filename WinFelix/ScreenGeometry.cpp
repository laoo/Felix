#include "pch.hpp"
#include "ScreenGeometry.hpp"

ScreenGeometry::ScreenGeometry() : mWinWidth{}, mWinHeight{}, mScale{ 1 }
{
}

ScreenGeometry::ScreenGeometry( int windowWidth, int windowHeight, ImageProperties::Rotation rotation ) : mWinWidth{ windowWidth }, mWinHeight{ windowHeight }, mScale{ 1 }, mRotation{ rotation }
{
  int sx, sy;
  switch ( mRotation )
  {
  case ImageProperties::Rotation::LEFT:
  case ImageProperties::Rotation::RIGHT:
    sx = (std::max)( 1, mWinWidth / 102 );
    sy = (std::max)( 1, mWinHeight / 160 );
    break;
  default:
    sx = (std::max)( 1, mWinWidth / 160 );
    sy = (std::max)( 1, mWinHeight / 102 );
    break;
  }

  mScale = (std::min)( sx, sy );
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
  return mRotation == ImageProperties::Rotation::NORMAL ? 160 : 102;
}

int ScreenGeometry::minWindowHeight() const
{
  return mRotation == ImageProperties::Rotation::NORMAL ? 102 : 160;
}

int ScreenGeometry::width() const
{
  return minWindowWidth();
}

int ScreenGeometry::height() const
{
  return minWindowHeight();
}

int ScreenGeometry::xOff() const
{
  switch ( mRotation )
  {
  case ImageProperties::Rotation::LEFT:
    return ( mWinWidth + 102 * mScale ) / 2;
  case ImageProperties::Rotation::RIGHT:
    return ( mWinWidth - 102 * mScale ) / 2;
  default:
    return ( mWinWidth - 160 * mScale ) / 2;
  }
}

int ScreenGeometry::yOff() const
{
  switch ( mRotation )
  {
  case ImageProperties::Rotation::LEFT:
    return ( mWinHeight - 160 * mScale ) / 2;
  case ImageProperties::Rotation::RIGHT:
    return ( mWinHeight + 160 * mScale ) / 2;
  default:
    return ( mWinHeight - 102 * mScale ) / 2;
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


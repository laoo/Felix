#pragma once
#include "ImageProperties.hpp"

class ScreenGeometry
{
public:
  ScreenGeometry();

  //returns whether something changed and is not empty
  bool update( int windowWidth, int windowHeight, ImageProperties::Rotation rotation );

  int windowWidth() const;
  int windowHeight() const;
  int minWindowWidth() const;
  int minWindowHeight() const;
  int width() const;
  int height() const;
  int xOff() const;
  int yOff() const;
  int scale() const;
  int rotx1() const;
  int rotx2() const;
  int roty1() const;
  int roty2() const;
  ImageProperties::Rotation rotation() const;
  explicit operator bool() const;

private:
  int mWinWidth;
  int mWinHeight;
  int mScale;
  ImageProperties::Rotation mRotation;
};


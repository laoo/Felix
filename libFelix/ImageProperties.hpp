#pragma once

class ImageProperties
{
public:
  enum class Rotation
  {
    NORMAL = 0,
    LEFT = 1,
    RIGHT = 2
  };

  ImageProperties();

  void setRotation( uint8_t rotation );

  Rotation getRotation() const;

private:
  Rotation mRotation;
};

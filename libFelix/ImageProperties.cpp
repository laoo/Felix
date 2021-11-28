#include "pch.hpp"
#include "ImageProperties.hpp"

ImageProperties::ImageProperties() : mRotation{}
{
}

void ImageProperties::setRotation( uint8_t rotation )
{
  mRotation = Rotation{ rotation };
}

ImageProperties::Rotation ImageProperties::getRotation() const
{
  return mRotation;
}

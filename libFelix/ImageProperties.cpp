#include "pch.hpp"
#include "ImageProperties.hpp"

ImageProperties::ImageProperties( std::filesystem::path const& path ) : mPath{ path }, mRotation {}, mEEPROM{}
{
}

void ImageProperties::setRotation( uint8_t rotation )
{
  mRotation = Rotation{ rotation };
}

void ImageProperties::setEEPROM( uint8_t eepromBits )
{
  mEEPROM = EEPROM{ eepromBits };
}

void ImageProperties::setCartridgeName( std::string_view name )
{
  mCartridgeName = name;
}

ImageProperties::Rotation ImageProperties::getRotation() const
{
  return mRotation;
}

ImageProperties::EEPROM ImageProperties::getEEPROM() const
{
  return mEEPROM;
}

std::filesystem::path ImageProperties::getPath() const
{
  return mPath;
}

std::string_view ImageProperties::getCartridgeName() const
{
  return mCartridgeName;
}

bool ImageProperties::EEPROM::sd() const
{
  return ( bits & 0x40 ) != 0;
}

int ImageProperties::EEPROM::type() const
{
  return bits & 7;
}

bool ImageProperties::EEPROM::is16Bit() const
{
  return ( bits & 0x80 ) == 0;
}

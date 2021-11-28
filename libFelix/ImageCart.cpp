#include "pch.hpp"
#include "ImageCart.hpp"

CartBank ImageCart::getBank0() const
{
  return mBank0;
}

CartBank ImageCart::getBank0A() const
{
  return mBank0A;
}

CartBank ImageCart::getBank1() const
{
  return mBank1;
}

CartBank ImageCart::getBank1A() const
{
  return mBank1A;
}

ImageCart::EEPROM ImageCart::eeprom() const
{
  return mEEPROM;
}

ImageProperties::Rotation ImageCart::rotation() const
{
  return mRotation;
}

std::filesystem::path ImageCart::path() const
{
  return mImagePath;
}

ImageCart::ImageCart( std::vector<uint8_t> data, std::filesystem::path path ) : mImagePath{ std::move( path ) }, mData { std::move( data ) },
  mBank0{}, mBank0A{}, mBank1{}, mBank1A{}, mEEPROM{}, mRotation{}
{
}

bool ImageCart::EEPROM::sd() const
{
  return ( bits & 0x40 ) != 0;
}

int ImageCart::EEPROM::type() const
{
  return bits & 7;
}

bool ImageCart::EEPROM::is16Bit() const
{
  return ( bits & 0x80 ) == 0;
}

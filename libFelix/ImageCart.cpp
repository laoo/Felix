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

bool ImageCart::sd() const
{
  return mSD;
}

bool ImageCart::eeprom() const
{
  return mEEPROM;
}

std::filesystem::path ImageCart::path() const
{
  return mImagePath;
}

ImageCart::ImageCart( std::vector<uint8_t> data, std::filesystem::path path ) : mImagePath{ std::move( path ) }, mData { std::move( data ) },
  mBank0{}, mBank0A{}, mBank1{}, mBank1A{}, mSD{}, mEEPROM{}
{
}

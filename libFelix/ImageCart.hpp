#pragma once

#include "CartBank.hpp"

class ImageCart
{
public:
  ImageCart( std::vector<uint8_t> data = {}, std::filesystem::path path = {} );

  CartBank getBank0() const;
  CartBank getBank0A() const;
  CartBank getBank1() const;
  CartBank getBank1A() const;

  bool sd() const;
  bool eeprom() const;
  std::filesystem::path path() const;

protected:

  std::filesystem::path mImagePath;
  std::vector<uint8_t> const mData;
  CartBank mBank0;
  CartBank mBank0A;
  CartBank mBank1;
  CartBank mBank1A;
  bool mSD;
  bool mEEPROM;
};

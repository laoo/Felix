#pragma once

#include "CartBank.hpp"

class ImageCart
{
public:

  struct EEPROM
  {
    uint8_t bits;

    bool sd() const;
    int type() const;
    bool is16Bit() const;
  };

  enum class Rotation
  {
    NORMAL = 0,
    LEFT = 1,
    RIGHT = 2
  };

  ImageCart( std::vector<uint8_t> data = {}, std::filesystem::path path = {} );

  CartBank getBank0() const;
  CartBank getBank0A() const;
  CartBank getBank1() const;
  CartBank getBank1A() const;

  EEPROM eeprom() const;
  Rotation rotation() const;
  std::filesystem::path path() const;

protected:

  std::filesystem::path mImagePath;
  std::vector<uint8_t> const mData;
  CartBank mBank0;
  CartBank mBank0A;
  CartBank mBank1;
  CartBank mBank1A;
  EEPROM mEEPROM;
  Rotation mRotation;
};

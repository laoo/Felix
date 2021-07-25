#pragma once

#include "ImageCart.hpp"

class ImageLnx : public ImageCart
{
public:

  struct Header
  {
    std::array<uint8_t, 4>   magic;
    uint16_t                 pageSizeBank0;
    uint16_t                 pageSizeBank1;
    uint16_t                 version;
    std::array<uint8_t, 32>  cartname;
    std::array<uint8_t, 16>  manufname;
    uint8_t                  rotation;
    uint8_t                  audBits;
    uint8_t                  eepromBits;
    std::array<uint8_t, 3>   spare;

    bool sd() const
    {
      return ( eepromBits & 0x40 ) != 0;
    }

    bool eeprom() const
    {
      return ( eepromBits & 0x01 ) != 0;
    }

  } header{};

  ImageLnx( std::filesystem::path const& path, std::vector<uint8_t> data );

private:
  Header const*const mHeader;
};

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
    uint8_t                  eeprom;
    std::array<uint8_t, 3>   spare;
  } header{};

  ImageLnx( std::vector<uint8_t> data );

private:
  Header const*const mHeader;
};

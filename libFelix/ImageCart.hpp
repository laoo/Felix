#pragma once

#include "CartBank.hpp"

class ImageProperties;

class ImageCart
{

  struct TagLnx {};
  struct TagLyx {};

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
  };

  static std::shared_ptr<ImageCart const> create( std::vector<uint8_t> & data );

  ImageCart( std::vector<uint8_t> data, TagLnx lnx );
  ImageCart( std::vector<uint8_t> data, TagLyx lyx );

  CartBank getBank0() const;
  CartBank getBank0A() const;
  CartBank getBank1() const;
  CartBank getBank1A() const;

  void populate( ImageProperties & imageProperties ) const;

private:

  static std::shared_ptr<ImageCart const> createLyx( std::vector<uint8_t> & data );
  static std::shared_ptr<ImageCart const> createLnx( std::vector<uint8_t> & data );

protected:

  std::vector<uint8_t> const mData;
  CartBank mBank0;
  CartBank mBank0A;
  CartBank mBank1;
  CartBank mBank1A;
  Header const*const mHeader;
};

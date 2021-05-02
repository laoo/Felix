#pragma once

#include "CartBank.hpp"

class ImageCart
{
public:
  ImageCart( std::vector<uint8_t> data = {} );

  CartBank getBank0() const;
  CartBank getBank0A() const;
  CartBank getBank1() const;
  CartBank getBank1A() const;

protected:

  std::vector<uint8_t> const mData;
  CartBank mBank0;
  CartBank mBank0A;
  CartBank mBank1;
  CartBank mBank1A;
};

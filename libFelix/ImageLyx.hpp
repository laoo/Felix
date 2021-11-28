#pragma once

#include "ImageCart.hpp"

class ImageLyx : public ImageCart
{
public:

  static std::shared_ptr<ImageCart const> create( std::vector<uint8_t> & data );

  ImageLyx( std::vector<uint8_t> data );

};

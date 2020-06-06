#include "ImageLyx.hpp"

ImageLyx::ImageLyx( std::vector<uint8_t> data ) : ImageCart{ std::move( data ) }
{
  mBank0 = { std::span<uint8_t const>( mData.data(), mData.size() ) };
}

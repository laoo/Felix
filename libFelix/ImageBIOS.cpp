#include "pch.hpp"
#include "ImageBIOS.hpp"
#include "MemoryUnit.hpp"

ImageBIOS::ImageBIOS( std::vector<uint8_t> data ) : mData{ std::move( data ) }
{
}

void ImageBIOS::load( std::span<MemU> memory ) const
{
  auto beg = mData.cbegin();
  auto end = mData.cend();

  assert( mData.size() == memory.size() );

  std::copy( beg, end, memory.begin() );
}

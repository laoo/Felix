#include "pch.hpp"
#include "ImageBIOS.hpp"
#include "Utility.hpp"

std::shared_ptr<ImageBIOS const> ImageBIOS::create( std::filesystem::path const& path )
{
  auto data = readFile( path );

  if ( data.size() != 512 )
    return {};

  uint16_t resetVector = *(uint16_t*)( data.data() + 0x1fc );

  if ( resetVector < 0xfe00 )
    return {};

  return std::make_shared<ImageBIOS const>( std::move( data ) );
}

ImageBIOS::ImageBIOS( std::vector<uint8_t> data ) : mData{ std::move( data ) }
{
}

void ImageBIOS::load( std::span<uint8_t> memory ) const
{
  auto beg = mData.cbegin();
  auto end = mData.cend();

  assert( mData.size() == memory.size() );

  std::copy( beg, end, memory.begin() );
}

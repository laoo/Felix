#include "pch.hpp"
#include "ImageLyx.hpp"
#include "Encryption.hpp"

std::shared_ptr<ImageCart const> ImageLyx::create( std::vector<uint8_t> & data )
{
  // First byte of loader has two's complement of number of blocks in first frame. 
  size_t blockcount = 0x100 - data[0];

  // If value is greater than 5 it is not a correct header
  if ( blockcount > 5 )
  {
    return {};
  }

  auto plain = decrypt( blockcount, std::span<uint8_t const>{ data.data() + 1, 51 * blockcount } );

  if ( plain.empty() )
    return {}; //not a valid cartridge image if decryption failed

  switch ( data.size() )
  {
  case 64 * 1024:
  case 128 * 1024:
  case 256 * 1024:
  case 512 * 1024:
    return std::make_shared<ImageLyx const>( std::move( data ) );
  default:
    return {};
  }
}


ImageLyx::ImageLyx( std::vector<uint8_t> data ) : ImageCart{ std::move( data ) }
{
  mBank0 = { std::span<uint8_t const>( mData.data(), mData.size() ) };
}

#include "pch.hpp"
#include "ImageBS93.hpp"

ImageBS93::ImageBS93( std::vector<uint8_t> data ) : mData{ std::move( data ) }
{
}

std::optional<uint16_t> ImageBS93::load( std::span<uint8_t> memory ) const
{
  uint16_t loadAddress = getLoadAddress();
  uint16_t size = getSize();
  uint16_t realLoadAddress = loadAddress - sizeof( Header );

  if ( realLoadAddress >= loadAddress )
    return std::nullopt;

  auto beg = mData.cbegin();
  auto end = mData.cend();

  size_t realSize = std::min( ( size_t )size, ( size_t )std::distance( beg, end ) );

  if ( realLoadAddress + realSize > memory.size() )
    return std::nullopt;

  auto dest = memory.begin() + realLoadAddress;

  std::copy_n( beg, realSize, dest );

  return realLoadAddress;
}

uint16_t ImageBS93::getLoadAddress() const
{
  auto const* pHeader = ( Header const* )mData.data();

  return ( ( uint16_t )pHeader->load_addressHi << 8 ) | pHeader->load_addressLo;
}

uint16_t ImageBS93::getSize() const
{
  auto const* pHeader = ( Header const* )mData.data();

  return ( ( uint16_t )pHeader->sizeHi << 8 ) | pHeader->sizeLo;
}

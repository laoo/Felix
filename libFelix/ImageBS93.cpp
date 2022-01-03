#include "pch.hpp"
#include "ImageBS93.hpp"

std::shared_ptr<ImageBS93 const> ImageBS93::create( std::vector<uint8_t> & data )
{
  auto const* pHeader = (ImageBS93::Header const*)data.data();

  if ( pHeader->magic[0] == 'B' && pHeader->magic[1] == 'S' && pHeader->magic[2] == '9' && pHeader->magic[3] == '3' )
  {
    return std::make_shared<ImageBS93 const>( std::move( data ) );
  }
  else
  {
    return {};
  }
}


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

  return realLoadAddress + 10; /* +10 => skip header */
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

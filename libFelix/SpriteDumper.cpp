#include "SpriteDumper.hpp"
#include "Utility.hpp"
#include <bit>
#include "stb_image_write.h"

namespace
{

//https://en.wikipedia.org/wiki/Fletcher%27s_checksum
uint32_t fletcher32( const uint16_t* data, size_t len )
{
  uint32_t c0, c1;
  len = ( len + 1 ) & ~1;      /* Round up len to words */

  /* We similarly solve for n > 0 and n * (n+1) / 2 * (2^16-1) < (2^32-1) here. */
  /* On modern computers, using a 64-bit c0/c1 could allow a group size of 23726746. */
  for ( c0 = c1 = 0; len > 0; )
  {
    size_t blocklen = len;
    if ( blocklen > 360 * 2 )
    {
      blocklen = 360 * 2;
    }
    len -= blocklen;
    do
    {
      c0 = c0 + *data++;
      c1 = c1 + c0;
    }
    while ( ( blocklen -= 2 ) );
    c0 = c0 % 65535;
    c1 = c1 % 65535;
  }
  return ( c1 << 16 | c0 );
}

}


SpriteDumper::SpriteDumper( std::filesystem::path outputPath ) : mOutputPath{ std::move( outputPath ) }, mPalette{}
{
  std::ranges::fill( mScreen, 0 );
}

void SpriteDumper::setPalette( std::span<uint8_t const> palette )
{
  for ( size_t i = 0; i < 16; ++i )
  {
    Pixel value;
    value.x = 0xff;
    value.r = ( palette[i + 16] & 0x0f );
    value.r |= value.r << 4;
    value.g = ( palette[i] & 0x0f );
    value.g |= value.g << 4;
    value.b = ( palette[i + 16] & 0xf0 );
    value.b |= value.b >> 4;

    mPalette[i] = value.toRGBA();
  }
}

void SpriteDumper::startSprite( uint16_t windowAddress, int16_t posx, int16_t posy, bool background )
{
  mWindowAddress = windowAddress;
  mBackground = background;
  mNewSprite = true;

  if ( mData.empty() )
    return;

  size_t size = mData.size();

  if ( ( size & 1 ) == 1 )
  {
    mData.push_back( 0 );
  }

  assert( mData.size() % 2 == 0 );

  uint32_t checksum = fletcher32( std::bit_cast< uint16_t const* >( mData.data() ), mData.size() / 2 );

  char buf[16];

  auto it = std::ranges::lower_bound( mProcessedSprites, checksum, std::ranges::less{}, &Desc::crc );
  if ( it != mProcessedSprites.cend() && it->crc == checksum )
  {
    if ( it->width() >= mCurrectDesc.width() && it->height() >= mCurrectDesc.height() );
    {
      mData.clear();
      std::ranges::fill( mScreen, 0 );
      return;
    }
    mCurrectDesc.id = it->id;
  }
  else
  {
    mCurrectDesc.id = (uint32_t)mProcessedSprites.size();
    mCurrectDesc.crc = checksum;
    mProcessedSprites.insert( it, mCurrectDesc );
    //sprintf( buf, "%08d", mCurrectDesc.id );
    //auto path = mOutputPath / std::string{ buf };
    //path.replace_extension( ".spr" );

    //std::ofstream fout{ path, std::ios::binary };
    //fout.write( std::bit_cast< char const* >( mData.data() ), size );
  }

  sprintf( buf, "%08d", mCurrectDesc.id );
  auto path = mOutputPath / std::string{ buf };
  path.replace_extension( ".bmp" );
  createPNG( path );

  mData.clear();
  std::ranges::fill( mScreen, 0 );
}

uint8_t SpriteDumper::fetch( uint8_t value )
{
  mData.push_back( value );
  return value;
}

uint32_t SpriteDumper::fetch( uint32_t value )
{
  mData.push_back( ( value >> 0 ) & 0xff );
  mData.push_back( ( value >> 8 ) & 0xff );
  mData.push_back( ( value >> 16 ) & 0xff );
  mData.push_back( ( value >> 24 ) & 0xff );
  return value;
}

void SpriteDumper::drawByte( uint16_t address, uint8_t value, uint8_t mask )
{
  uint32_t off = address - mWindowAddress;
  auto pos = pixelPos( off );
  if ( mNewSprite )
  {
    mCurrectDesc.minx = pos.first;
    mCurrectDesc.miny = pos.second;
    mCurrectDesc.maxx = (uint8_t)( pos.first + 1 );
    mCurrectDesc.maxy = pos.second;
    mNewSprite = false;
  }
  else
  {
    mCurrectDesc.minx = std::min( mCurrectDesc.minx, pos.first );
    mCurrectDesc.miny = std::min( mCurrectDesc.miny, pos.second );
    mCurrectDesc.maxx = std::max( mCurrectDesc.maxx, (uint8_t)( pos.first + 1 ) );
    mCurrectDesc.maxy = std::max( mCurrectDesc.maxy, pos.second );
  }


  value &= ~mask;
  int left = value >> 4;
  int right = value & 0xf;
  
  mScreen[off * 2] = left || mBackground ? mPalette[left] : 0;
  mScreen[off * 2 + 1] = right || mBackground ? mPalette[right] : 0;
}

void SpriteDumper::createPNG( std::filesystem::path outputPath )
{
  std::vector<uint32_t> data;
  data.reserve( mCurrectDesc.height() * mCurrectDesc.width() );

  for ( int y = mCurrectDesc.miny; y <= mCurrectDesc.maxy; ++y )
  {
    for ( int x = mCurrectDesc.minx; x <= mCurrectDesc.maxx; ++x )
    {
      data.push_back( mScreen[y * 160 + x] );
    }
  }

  stbi_write_bmp( outputPath.string().c_str(), mCurrectDesc.width(), mCurrectDesc.height(), 4, ( void const* )data.data() );
}

std::pair<uint8_t, uint8_t> SpriteDumper::pixelPos( uint32_t off ) const
{
  return std::pair<uint8_t, uint8_t>( ( off % 80 ) * 2, off / 80 );
}


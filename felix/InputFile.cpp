#include "pch.hpp"
#include "InputFile.hpp"
#include "ImageBS93.hpp"
#include "ImageBIOS.hpp"
#include "ImageLyx.hpp"
#include "ImageLnx.hpp"
#include "ImageBIN.hpp"
#include "Encryption.hpp"
#include "Log.hpp"

InputFile::InputFile( std::filesystem::path const & path ) : mType{}, mBS93{}
{
  if ( !std::filesystem::exists( path ) )
    return;

  size_t size = ( size_t )std::filesystem::file_size( path );
  if ( size == 0 )
    return;

  std::vector<uint8_t> data( size, 0 );

  {
    std::ifstream fin{ path, std::ios::binary };
    fin.read( ( char* )data.data(), size );
  }

  if ( auto pBIOS = checkBIOS( std::move( data ) ) )
  {
    mType = FileType::BIOS;
    mBIOS = std::move( pBIOS );
    return;
  }
  else if ( auto pBS93 = checkBS93( std::move( data ) ) )
  {
    mType = FileType::BS93;
    mBS93 = std::move( pBS93 );
    return;
  }
  else if ( auto pLyx = checkLyx( std::move( data ) ) )
  {
    mType = FileType::CART;
    mCart = std::move( pLyx );
    return;
  }
  else if ( auto pLnx = checkLnx( std::move( data ) ) )
  {
    mType = FileType::CART;
    mCart = std::move( pLnx );
    return;
  }
  else if ( auto pBIN = checkBIN( std::move( data ) ) )
  {
    mType = FileType::BIN;
    mBIN = std::move( pBIN );
    return;
  }

}

bool InputFile::valid() const
{
  return mType != FileType::UNKNOWN;
}

InputFile::FileType InputFile::getType() const
{
  return mType;
}

std::shared_ptr<ImageBS93 const> InputFile::getBS93() const
{
  return mBS93;
}

std::shared_ptr<ImageBIOS const> InputFile::getBIOS() const
{
  return mBIOS;
}

std::shared_ptr<ImageCart const> InputFile::getCart() const
{
  return mCart;
}

std::shared_ptr<ImageBIN const> InputFile::getBIN() const
{
  return mBIN;
}

std::shared_ptr<ImageBS93 const> InputFile::checkBS93( std::vector<uint8_t>&& data ) const
{
  auto const* pHeader = ( ImageBS93::Header const* )data.data();

  if ( pHeader->magic[0] == 'B' && pHeader->magic[1] == 'S' && pHeader->magic[2] == '9' && pHeader->magic[3] == '3' )
  {
    return std::make_shared<ImageBS93 const>( std::move( data ) );
  }
  else
  {
    return {};
  }
}

std::shared_ptr<ImageBIOS const> InputFile::checkBIOS( std::vector<uint8_t>&& data ) const
{
  if ( data.size() != 512 )
    return {};

  uint16_t resetVector = *(uint16_t*)( data.data() + 0x1fc );

  if ( resetVector < 0xfe00 )
    return {};

  return std::make_shared<ImageBIOS const>( std::move( data ) );
}

std::shared_ptr<ImageCart const> InputFile::checkLnx( std::vector<uint8_t>&& data ) const
{
  auto const* pHeader = ( ImageLnx::Header const* )data.data();

  if ( pHeader->magic[0] == 'L' && pHeader->magic[1] == 'Y' && pHeader->magic[2] == 'N' && pHeader->magic[3] == 'X' && pHeader->version == 1 )
  {
    return std::make_shared<ImageLnx const>( std::move( data ) );
  }
  else
  {
    return {};
  }
}

std::shared_ptr<ImageBIN const> InputFile::checkBIN( std::vector<uint8_t>&& data ) const
{
  if ( data.size() != 65536 )
    return {};

  return std::make_shared<ImageBIN const>( std::move( data ) );
}


std::shared_ptr<ImageCart const> InputFile::checkLyx( std::vector<uint8_t>&& data ) const
{
  // First byte of loader has two's complement of number of blocks in first frame. 
  size_t blockcount = 0x100 - data[0];

  // If value is greater than 5 it is not a correct header
  if ( blockcount > 5 )
  {
    L_ERROR << "Bad number of encrypted blocks: " << blockcount;
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


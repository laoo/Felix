#include "InputFile.hpp"
#include <fstream>
#include "ImageBS93.hpp"
#include "ImageBIOS.hpp"

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

}

bool InputFile::valid() const
{
  return mType != FileType::UNKNOWN;
}

InputFile::FileType InputFile::getType() const
{
  return mType;
}

std::shared_ptr<ImageBS93> InputFile::getBS93() const
{
  return mBS93;
}

std::shared_ptr<ImageBIOS> InputFile::getBIOS() const
{
  return mBIOS;
}

std::shared_ptr<ImageBS93> InputFile::checkBS93( std::vector<uint8_t>&& data ) const
{
  auto const* pHeader = ( ImageBS93::Header const* )data.data();

  if ( pHeader->magic[0] == 'B' && pHeader->magic[1] == 'S' && pHeader->magic[2] == '9' && pHeader->magic[3] == '3' )
  {
    return std::make_shared<ImageBS93>( std::move( data ) );
  }
  else
  {
    return {};
  }
}

std::shared_ptr<ImageBIOS> InputFile::checkBIOS( std::vector<uint8_t>&& data ) const
{
  if ( data.size() != 512 )
    return {};

  uint16_t resetVector = *(uint16_t*)( data.data() + 0x1fc );

  if ( resetVector < 0xfe00 )
    return {};


  return std::make_shared<ImageBIOS>( std::move( data ) );
}


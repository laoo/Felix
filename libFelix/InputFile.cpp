#include "pch.hpp"
#include "InputFile.hpp"
#include "ImageBS93.hpp"
#include "ImageBIOS.hpp"
#include "ImageLyx.hpp"
#include "ImageLnx.hpp"
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

  if ( auto pLnx = ImageLnx::create( path, data ) )
  {
    mType = FileType::CART;
    mCart = std::move( pLnx );
    return;
  }
  else if ( auto pBS93 = ImageBS93::create( data ) )
  {
    mType = FileType::BS93;
    mBS93 = std::move( pBS93 );
    return;
  }
  else if ( auto pLyx = ImageLyx::create( data ) )
  {
    mType = FileType::CART;
    mCart = std::move( pLyx );
    return;
  }
  else if ( auto pBIOS = ImageBIOS::create( data ) )
  {
    mType = FileType::BIOS;
    mBIOS = std::move( pBIOS );
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




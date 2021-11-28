#include "pch.hpp"
#include "InputFile.hpp"
#include "ImageBS93.hpp"
#include "ImageBIOS.hpp"
#include "ImageLyx.hpp"
#include "ImageLnx.hpp"
#include "Utility.hpp"
#include "Log.hpp"

InputFile::InputFile( std::filesystem::path const & path, ImageProperties & imageProperties ) : mType{}, mBS93{}, mCart{}
{
  auto data = readFile( path );

  if ( auto pLnx = ImageLnx::create( path, data ) )
  {
    pLnx->populate( imageProperties );
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

std::shared_ptr<ImageCart const> InputFile::getCart() const
{
  return mCart;
}




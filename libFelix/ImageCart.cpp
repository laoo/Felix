#include "ImageCart.hpp"
#include "ImageProperties.hpp"
#include "Encryption.hpp"

std::shared_ptr<ImageCart const> ImageCart::create( std::vector<uint8_t>& data )
{
  if ( auto pLnx = createLnx( data ) )
  {
    return pLnx;
  }
  else if ( auto pLyx = createLyx( data ) )
  {
    return pLyx;
  }
  else
  {
    return {};
  }
}

ImageCart::ImageCart( std::vector<uint8_t> data, TagLnx lnx ) : mData{ std::move( data ) },
  mBank0{}, mBank0A{}, mBank1{}, mBank1A{}, mHeader{ (Header const*)mData.data() }
{
  auto const* pImageData = mData.data() + sizeof( Header );
  size_t imageDataSize = mData.size() - sizeof( Header );

  size_t bank0Offset = 0;
  size_t bank0Size = std::min( imageDataSize, (size_t)mHeader->pageSizeBank0 * 256 );
  size_t bank1Offset = bank0Offset + bank0Size;
  size_t bank1Size = std::min( imageDataSize - bank0Size, (size_t)mHeader->pageSizeBank1 * 256 );
  size_t bank0AOffset = bank1Offset + bank1Size;
  size_t bank0ASize = std::min( imageDataSize - bank0Size - bank1Size, (size_t)mHeader->pageSizeBank0 * 256 );
  size_t bank1AOffset = bank0AOffset + bank0ASize;
  size_t bank1ASize = std::min( imageDataSize - bank0Size - bank1Size - bank0ASize, (size_t)mHeader->pageSizeBank1 * 256 );

  if ( bank0Size )
    mBank0 = { std::span<uint8_t const>{ pImageData + bank0Offset, bank0Size }, (uint32_t)mHeader->pageSizeBank0 * 256 };
  if ( bank1Size )
    mBank1 = { std::span<uint8_t const>{ pImageData + bank1Offset, bank1Size }, (uint32_t)mHeader->pageSizeBank1 * 256 };
  if ( bank0Size )
    mBank0A = { std::span<uint8_t const>{ pImageData + bank0AOffset, bank0ASize }, (uint32_t)mHeader->pageSizeBank0 * 256 };
  if ( bank1Size )
    mBank1A = { std::span<uint8_t const>{ pImageData + bank1AOffset, bank1ASize }, (uint32_t)mHeader->pageSizeBank1 * 256 };
}

ImageCart::ImageCart( std::vector<uint8_t> data, TagLyx lyx ) : mData{ std::move( data ) },
  mBank0{ std::span<uint8_t const>( mData.data(), mData.size() ) }, mBank0A{}, mBank1{}, mBank1A{}, mHeader{}
{
}

CartBank ImageCart::getBank0() const
{
  return mBank0;
}

CartBank ImageCart::getBank0A() const
{
  return mBank0A;
}

CartBank ImageCart::getBank1() const
{
  return mBank1;
}

CartBank ImageCart::getBank1A() const
{
  return mBank1A;
}

std::shared_ptr<ImageCart const> ImageCart::createLyx( std::vector<uint8_t>& data )
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
    return std::make_shared<ImageCart const>( std::move( data ), TagLyx{} );
  default:
    return {};
  }
}

std::shared_ptr<ImageCart const> ImageCart::createLnx( std::vector<uint8_t>& data )
{
  auto const* pHeader = (Header const*)data.data();

  if ( pHeader->magic[0] == 'L' && pHeader->magic[1] == 'Y' && pHeader->magic[2] == 'N' && pHeader->magic[3] == 'X' && pHeader->version == 1 )
  {
    return std::make_shared<ImageCart const>( std::move( data ), TagLnx{} );
  }
  else
  {
    return {};
  }
}

void ImageCart::populate( ImageProperties & imageProperties ) const
{
  if ( mHeader )
  {
    imageProperties.setRotation( mHeader->rotation );
    imageProperties.setEEPROM( mHeader->eepromBits );
    imageProperties.setCartridgeName( std::string_view{ mHeader->cartname.data(), std::min( mHeader->cartname.size(), std::strlen( (char const*)mHeader->cartname.data() ) ) } );
    imageProperties.setMamufacturerName( std::string_view{ mHeader->manufname.data(), std::min( mHeader->manufname.size(), std::strlen( (char const*)mHeader->manufname.data() ) ) } );
    imageProperties.setAUDInUsed( mHeader->audBits != 0 );
  }

  imageProperties.setBankProps( std::array<ImageProperties::BankProps, 4>{
      ImageProperties::BankProps{ mBank0.pageSize(), mBank0.numberOfPages() },
      ImageProperties::BankProps{ mBank0A.pageSize(), mBank0A.numberOfPages() },
      ImageProperties::BankProps{ mBank1.pageSize(), mBank1.numberOfPages() },
      ImageProperties::BankProps{ mBank1A.pageSize(), mBank1A.numberOfPages() }
    } );
}
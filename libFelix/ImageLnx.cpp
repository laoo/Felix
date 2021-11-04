#include "pch.hpp"
#include "ImageLnx.hpp"

ImageLnx::ImageLnx( std::filesystem::path const& path, std::vector<uint8_t> data ) : ImageCart{ std::move( data ), path }, mHeader{ ( Header const* )mData.data() }
{
  auto const* pImageData = mData.data() + sizeof( Header );
  size_t imageDataSize = mData.size() - sizeof( Header );

  size_t bank0Offset = 0;
  size_t bank0Size = std::min( imageDataSize, ( size_t )mHeader->pageSizeBank0 * 256 );
  size_t bank1Offset = bank0Offset + bank0Size;
  size_t bank1Size = std::min( imageDataSize - bank0Size, ( size_t )mHeader->pageSizeBank1 * 256 );
  size_t bank0AOffset = bank1Offset + bank1Size;
  size_t bank0ASize = std::min( imageDataSize - bank0Size - bank1Size, ( size_t )mHeader->pageSizeBank0 * 256 );
  size_t bank1AOffset = bank0AOffset + bank0ASize;
  size_t bank1ASize = std::min( imageDataSize - bank0Size - bank1Size - bank0ASize, ( size_t )mHeader->pageSizeBank1 * 256 );

  if ( bank0Size )
    mBank0 ={ std::span<uint8_t const>{ pImageData + bank0Offset, bank0Size }, ( uint32_t )mHeader->pageSizeBank0 * 256 };
  if ( bank1Size )
    mBank1 ={ std::span<uint8_t const>{ pImageData + bank1Offset, bank1Size }, ( uint32_t )mHeader->pageSizeBank1 * 256 };
  if ( bank0Size )
    mBank0A ={ std::span<uint8_t const>{ pImageData + bank0AOffset, bank0ASize }, ( uint32_t )mHeader->pageSizeBank0 * 256 };
  if ( bank1Size )
    mBank1A ={ std::span<uint8_t const>{ pImageData + bank1AOffset, bank1ASize }, ( uint32_t )mHeader->pageSizeBank1 * 256 };

  mEEPROM = ImageCart::EEPROM{ mHeader->eepromBits };
  mRotation = ImageCart::Rotation{ mHeader->rotation };
}

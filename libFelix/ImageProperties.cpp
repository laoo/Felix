#include "ImageProperties.hpp"

ImageProperties::ImageProperties( std::filesystem::path const& path ) : mPath{ path }, mCartridgeName{}, mMamufacturerName{}, mRotation{}, mEEPROM{}, mBankProps{}, mAUDInUsed{}
{
}

void ImageProperties::setRotation( uint8_t rotation )
{
  mRotation = Rotation{ rotation };
}

void ImageProperties::setEEPROM( uint8_t eepromBits )
{
  mEEPROM = EEPROM{ eepromBits };
}

void ImageProperties::setCartridgeName( std::string_view name )
{
  mCartridgeName = name;
}

void ImageProperties::setMamufacturerName( std::string_view name )
{
  mMamufacturerName = name;
}

void ImageProperties::setAUDInUsed( bool used )
{
  mAUDInUsed = used;
}

void ImageProperties::setBankProps( std::array<BankProps, 4> const& props )
{
  mBankProps = props;
}

ImageProperties::EEPROM& ImageProperties::eeprom()
{
  return mEEPROM;
}

ImageProperties::Rotation ImageProperties::getRotation() const
{
  return mRotation;
}

ImageProperties::EEPROM ImageProperties::getEEPROM() const
{
  return mEEPROM;
}

std::filesystem::path ImageProperties::getPath() const
{
  return mPath;
}

std::string_view ImageProperties::getCartridgeName() const
{
  return mCartridgeName;
}

std::string_view ImageProperties::getMamufacturerName() const
{
  return mMamufacturerName;
}

bool ImageProperties::getAUDInUsed() const
{
  return mAUDInUsed;
}

std::array<ImageProperties::BankProps, 4> const& ImageProperties::getBankProps() const
{
  return mBankProps;
}

bool ImageProperties::EEPROM::sd() const
{
  return ( bits & 0x40 ) != 0;
}

int ImageProperties::EEPROM::type() const
{
  return bits & 7;
}

bool ImageProperties::EEPROM::is16Bit() const
{
  return ( bits & 0x80 ) == 0;
}

void ImageProperties::EEPROM::setType( int type )
{
  bits &= ~7;
  bits |= type & 7;
}

void ImageProperties::EEPROM::setSD( bool sd )
{
  bits &= ~0x40;
  bits |= sd ? 0x40 : 0;
}

void ImageProperties::EEPROM::set16bit( bool is16bit )
{
  bits &= ~0x80;
  bits |= is16bit ? 0 : 0x80;
}


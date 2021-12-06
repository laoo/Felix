#pragma once

class ImageProperties
{
public:
  enum class Rotation
  {
    NORMAL = 0,
    LEFT = 1,
    RIGHT = 2
  };

  struct EEPROM
  {
    uint8_t bits;

    bool sd() const;
    int type() const;
    bool is16Bit() const;
  };

  ImageProperties( std::filesystem::path const& path );

  void setRotation( uint8_t rotation );
  void setEEPROM( uint8_t eepromBits );
  void setCartridgeName( std::string_view name );

  Rotation getRotation() const;
  EEPROM getEEPROM() const;
  std::filesystem::path getPath() const;
  std::string_view getCartridgeName() const;

private:
  std::filesystem::path mPath;
  std::string mCartridgeName;
  Rotation mRotation;
  EEPROM mEEPROM;
};

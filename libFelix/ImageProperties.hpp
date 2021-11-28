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

  Rotation getRotation() const;
  EEPROM getEEPROM() const;
  std::filesystem::path getPath() const;

private:
  std::filesystem::path mPath;
  Rotation mRotation;
  EEPROM mEEPROM;
};

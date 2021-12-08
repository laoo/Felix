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
    std::string_view name() const;

    void setType( int type );
    void setSD( bool sd );
    void set16bit( bool is16bit );
  };

  struct BankProps
  {
    uint16_t pageSize;
    uint16_t numberOfPages;
  };

  ImageProperties( std::filesystem::path const& path );

  void setRotation( uint8_t rotation );
  void setEEPROM( uint8_t eepromBits );
  void setCartridgeName( std::string_view name );
  void setMamufacturerName( std::string_view name );
  void setAUDInUsed( bool used );
  void setBankProps( std::array<BankProps, 4> const& props );

  EEPROM& eeprom();

  Rotation getRotation() const;
  EEPROM getEEPROM() const;
  std::filesystem::path getPath() const;
  std::string_view getCartridgeName() const;
  std::string_view getMamufacturerName() const;
  bool getAUDInUsed() const;
  std::array<BankProps, 4> const& getBankProps() const;

private:
  std::filesystem::path mPath;
  std::string mCartridgeName;
  std::string mMamufacturerName;
  Rotation mRotation;
  EEPROM mEEPROM;
  std::array<BankProps, 4> mBankProps;
  bool mAUDInUsed;


};

#pragma once

class ImageBS93
{
public:
  struct Header
  {
    uint16_t jump;
    uint8_t load_addressHi;
    uint8_t load_addressLo;
    uint8_t sizeHi;
    uint8_t sizeLo;
    uint8_t magic[4];
  };

public:

  static std::shared_ptr<ImageBS93 const> create( std::vector<uint8_t> & data );
  std::optional<uint16_t> load( std::span<uint8_t> memory ) const;
  ImageBS93( std::vector<uint8_t> data );

private:
  uint16_t getLoadAddress() const;
  uint16_t getSize() const;

private:
  std::vector<uint8_t> const mData;
};

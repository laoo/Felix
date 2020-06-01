#pragma once

#include <cstdint>
#include <vector>
#include <optional>

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
  ImageBS93( std::vector<uint8_t> data );

  std::optional<uint16_t> load( uint8_t * memory ) const;

private:
  uint16_t getLoadAddress() const;
  uint16_t getSize() const;

private:
  std::vector<uint8_t> const mData;
};

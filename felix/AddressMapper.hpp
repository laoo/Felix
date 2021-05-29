#pragma once

class AddressMapper
{
public:
  AddressMapper();
  char const * addressLabel( uint16_t address ) const;

private:
  char const * map( uint16_t address, char * dest ) const;

private:
  std::array<uint32_t, 65536> mLabels;
  std::vector<char> mData;
};


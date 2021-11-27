#pragma once

class KeyNames
{
public:
  KeyNames();
  char const* name( uint32_t Key ) const;

private:
  std::array<char const*, 256> mNames;

};

#pragma once

#include <cstdint>

struct MemU
{
  uint8_t value;

  operator uint8_t() const
  {
    return value;
  }

  MemU& operator=( uint8_t v )
  {
    value = v;
    return *this;
  }
};

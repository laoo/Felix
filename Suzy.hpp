#pragma once

#include <cstdint>

class Suzy
{
public:
  Suzy();

  uint64_t requestAccess( uint64_t tick, uint16_t address );
  uint8_t read( uint16_t address );
  void write( uint16_t address, uint8_t value );

};


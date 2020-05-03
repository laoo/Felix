#pragma once

#include <cstdint>

class Mikey
{
public:
  Mikey();

  uint64_t requestAccess( uint64_t tick, uint16_t address );
  uint8_t read( uint16_t address );
  void write( uint16_t address, uint8_t value );

  static constexpr uint16_t IODIR = 0xfd8a;
  static constexpr uint16_t IODAT = 0xfd8b;

  private:

    struct Regs
    {
      uint8_t iodir;
      uint8_t iodat;
    } mRegs;


};

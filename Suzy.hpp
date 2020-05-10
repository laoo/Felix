#pragma once
#include <cstdint>
#include "ActionQueue.hpp"

class Suzy
{
public:
  Suzy();

  uint64_t requestAccess( uint64_t tick, uint16_t address );
  uint8_t read( uint16_t address );
  SequencedAction write( uint16_t address, uint8_t value );


  static constexpr uint16_t HOFF        = 0x04;
  static constexpr uint16_t VOFF        = 0x06;
  static constexpr uint16_t COLLOFF     = 0x24;
  static constexpr uint16_t HSIZOFF     = 0x28;
  static constexpr uint16_t VSIZOFF     = 0x2a;
  static constexpr uint16_t SPRINIT     = 0x83;
  static constexpr uint16_t SUZYHREV    = 0x88;
  static constexpr uint16_t SUZYBUSEN   = 0x90;
  static constexpr uint16_t SPRSYS      = 0x92;

};


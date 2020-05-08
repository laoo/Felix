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

  static constexpr uint16_t SUZYHREV = 0xfc88;

};


#pragma once

class Core;

class IMemoryAccessTrap
{
public:
  virtual ~IMemoryAccessTrap() = 0;

  virtual uint8_t trap( Core& core, uint16_t address, uint8_t orgValue ) = 0;
};

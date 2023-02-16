#pragma once

class Core;

class IMemoryAccessTrap
{
public:

  typedef int Kind;
  static constexpr Kind UNKNOWN = 0b000;
  static constexpr Kind ROM_HLE = 0b001;
  static constexpr Kind LUA     = 0b010;
  static constexpr Kind UI      = 0b100;

  virtual ~IMemoryAccessTrap() = default;

  virtual uint8_t trap( Core& core, uint16_t address, uint8_t orgValue ) = 0;

  virtual Kind getKind() const = 0;
};

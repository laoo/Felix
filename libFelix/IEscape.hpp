#pragma once

struct CPUState;

class IEscape
{
public:
  class IAccessor
  {
  public:
    virtual uint8_t readRAM( uint16_t address ) const = 0;
    virtual void writeRAM( uint16_t address, uint8_t value ) = 0;

    virtual uint8_t readMikey( uint16_t address ) const = 0;
    virtual void writeMikey( uint16_t address, uint8_t value ) = 0;

    virtual uint8_t readSuzy( uint16_t address ) const = 0;
    virtual void writeSuzy( uint16_t address, uint8_t value ) = 0;

    virtual CPUState & state() = 0;
  };

  virtual ~IEscape() = default;

  virtual void call( uint8_t data, IAccessor & accessor ) = 0;
};


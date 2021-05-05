#pragma once

class IPatch
{
public:
  class IAccessor
  {
  public:
    virtual uint8_t read( uint16_t address ) const = 0;
    virtual void write( uint16_t address, uint8_t value ) = 0;
  };

  virtual ~IPatch() = default;

  virtual void call( uint8_t data, IAccessor & ram, IAccessor & mikey, IAccessor & suzy ) = 0;
};


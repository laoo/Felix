#pragma once

class CartBank;

class CustomCart
{
public:
  virtual ~CustomCart() = default;

  virtual bool hasOutput( uint64_t tick ) const
  {
    return false;
  }
  virtual void put( uint64_t tick, uint8_t value )
  {
  }
  virtual CartBank* getBank( uint64_t tick ) const
  {
    return nullptr;
  }

  virtual uint8_t get( uint64_t tick ) = 0;
};

#pragma once

#include <cstdint>
#include <gsl/span>

class CartBank
{
public:
  CartBank( gsl::span<uint8_t const> data = {} );
  bool empty() const;

  uint8_t operator()( uint32_t shiftRegister, uint32_t count ) const;

private:
  gsl::span<uint8_t const> mData;
  uint32_t mShift;

};


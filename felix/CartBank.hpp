#pragma once

#include <cstdint>
#include <gsl/span>
#include <optional>

class CartBank
{
public:
  CartBank( gsl::span<uint8_t const> data = {}, std::optional<uint32_t> declaredSize = std::nullopt );
  bool empty() const;
  size_t size() const;


  uint8_t operator()( uint32_t shiftRegister, uint32_t count ) const;

private:
  gsl::span<uint8_t const> mData;
  uint32_t mShift;

};


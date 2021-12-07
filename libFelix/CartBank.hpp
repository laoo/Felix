#pragma once

class CartBank
{
public:
  CartBank( std::span<uint8_t const> data = {}, std::optional<uint32_t> declaredSize = std::nullopt );
  bool empty() const;
  size_t size() const;
  uint16_t numberOfPages() const;
  uint16_t pageSize() const;


  uint8_t operator()( uint32_t shiftRegister, uint32_t count ) const;

private:
  std::span<uint8_t const> mData;
  uint32_t mShift;
  uint32_t mShiftMask;
  uint16_t mNumberOfPages;

};


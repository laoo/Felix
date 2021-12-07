#include "pch.hpp"
#include "CartBank.hpp"

namespace
{
constexpr uint32_t calcShift( int32_t v )
{
  uint32_t result = 0;
  v -= 1;

  while ( v >= 256 )
  {
    result += 1;
    v >>= 1;
  }

  return result;
}
}

CartBank::CartBank( std::span<uint8_t const> data, std::optional<uint32_t> declaredSize ) : mData{ std::move( data ) }, mShift{}, mShiftMask{}
{
  mShift = calcShift( declaredSize.value_or( ( int32_t )mData.size() ) );
  mShiftMask = ( 1 << mShift ) - 1;
  mNumberOfPages = (uint16_t)( ( (uint32_t)mData.size() + mShiftMask ) / ( mShiftMask + 1 ) );
}

uint8_t CartBank::operator()( uint32_t shiftRegister, uint32_t count ) const
{
  uint32_t address = ( shiftRegister << mShift ) + ( count & mShiftMask );

  return address < mData.size() ? mData[address] : 0xff;
}

bool CartBank::empty() const
{
  return mData.empty();
}

size_t CartBank::size() const
{
  return mData.size();
}

uint16_t CartBank::numberOfPages() const
{
  return mNumberOfPages;
}

uint16_t CartBank::pageSize() const
{
  return mShiftMask ? (uint16_t)( mShiftMask + 1 ) : 0;
}

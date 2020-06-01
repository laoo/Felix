#include "CartBank.hpp"
#include <cassert>

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

CartBank::CartBank( gsl::span<uint8_t const> data, std::optional<uint32_t> declaredSize ) : mData{ std::move( data ) }, mShift{}
{
  mShift = calcShift( declaredSize.value_or( ( int32_t )mData.size() ) );
}

uint8_t CartBank::operator()( uint32_t shiftRegister, uint32_t count ) const
{
  uint32_t address = ( shiftRegister << mShift ) + ( count & ( ( 1 << mShift ) - 1 ) );

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

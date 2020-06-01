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

CartBank::CartBank( gsl::span<uint8_t const> data ) : mData{ std::move( data ) }, mShift{ calcShift( ( int32_t )mData.size() ) }
{
}

uint8_t CartBank::operator()( uint32_t shiftRegister, uint32_t count ) const
{
  if ( mData.empty() )
    return 0xff;

  uint32_t address = ( shiftRegister << mShift ) + ( count & ( ( 1 << mShift ) - 1 ) );
  assert( address < mData.size() );

  return mData[address];
}

bool CartBank::empty() const
{
  return mData.empty();
}

#pragma once

#include <cstdint>
#include <type_traits>

class Shifter
{
public:

  Shifter() : mShifter{}, mSize{} {}

  template<int bits>
  int pull()
  {
    if ( mSize < bits )
      return mSize - bits;

    uint64_t result = mShifter >> ( 64 - bits );
    mShifter <<= bits;
    mSize -= bits;
    return ( int )result;
  }

  int pull( int bits )
  {
    if ( mSize < bits )
      return mSize - bits;

    uint64_t result = mShifter >> ( 64 - bits );
    mShifter <<= bits;
    mSize -= bits;
    return ( int )result;
  }

  void push( uint8_t value )
  {
    int offset = 64 - mSize - 8;
    mShifter |= (uint64_t)value << offset;
    mSize += 8;
  }

  template<typename T>
  bool push( T value )
  {
    static_assert( std::is_integral_v<T> );

    if (  mSize + ( int )sizeof( T ) <= 64 )
    {
      for ( int i = 0; i < sizeof( T ) * 8; i += 8 )
      {
        push( (uint8_t)( ( value >> i ) & 0xff ) );
      }
      return true;
    }

    return false;
  }

  int size() const
  {
    return mSize;
  }

private:
  uint64_t mShifter;
  ////number of bits in shifters
  int mSize;
};


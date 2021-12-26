#include "pch.hpp"
#include "CPUState.hpp"

uint8_t CPUState::inc( uint8_t val )
{
  uint8_t result = val + 1;
  setnz( result );
  return result;
}

uint8_t CPUState::dec( uint8_t val )
{
  uint8_t result = val - 1;
  setnz( result );
  return result;
}

uint8_t CPUState::asl( uint8_t val )
{
  c.set( val >= 0x80 );
  uint8_t result = val << 1;
  setnz( result );
  return result;
}

uint8_t CPUState::lsr( uint8_t val )
{
  c.set( ( val & 0x01 ) != 0 );
  uint8_t result = val >> 1;
  setnz( result );
  return result;
}

uint8_t CPUState::rol( uint8_t val )
{
  int roled = val << 1;
  uint8_t result = roled & 0xff | ( c ? 0x01 : 0 );
  setnz( result );
  c.set( ( roled & 0x100 ) != 0 );
  return result;
}

uint8_t CPUState::ror( uint8_t val )
{
  bool newC = ( val & 1 ) != 0;
  uint8_t result = ( val >> 1 ) | ( c ? 0x80 : 0 );
  setnz( result );
  c.set( newC );
  return result;
}

void CPUState::adc( uint8_t value )
{
  if ( d )
  {
    int lo = ( a & 0x0f ) + ( value & 0x0f ) + ( c ? 0x01 : 0 );
    int hi = ( a & 0xf0 ) + ( value & 0xf0 );
    v.clear();
    c.clear();
    if ( lo > 0x09 )
    {
      hi += 0x10;
      lo += 0x06;
    }
    if ( ~( a ^ value ) & ( a ^ hi ) & 0x80 )
    {
      v.set();
    }
    if ( hi > 0x90 )
    {
      hi += 0x60;
    }
    if ( hi & 0xff00 )
    {
      c.set();
    }
    a = ( lo & 0x0f ) + ( hi & 0xf0 );
    setnz( a );
  }
  else
  {
    int sum = a + value + ( c ? 0x01 : 0 );
    v.clear();
    c.clear();
    if ( ~( a ^ value ) & ( a ^ sum ) & 0x80 )
    {
      v.set();
    }
    if ( sum & 0xff00 )
    {
      c.set();
    }
    a = (uint8_t)sum;
    setnz( a );
  }
}

void CPUState::sbc( uint8_t value )
{
  int carry = c ? 0 : 1;
  if ( d )
  {
    int sum = a - value - carry;
    int lo = ( a & 0x0f ) - ( value & 0x0f ) - carry;
    int hi = ( a & 0xf0 ) - ( value & 0xf0 );
    v.clear();
    c.clear();
    if ( ( a ^ value ) & ( a ^ sum ) & 0x80 )
    {
      v.set();
    }
    if ( lo & 0xf0 )
    {
      lo -= 6;
    }
    if ( lo & 0x80 )
    {
      hi -= 0x10;
    }
    if ( hi & 0x0f00 )
    {
      hi -= 0x60;
    }
    if ( ( sum & 0xff00 ) == 0 )
    {
      c.set();
    }
    a = ( lo & 0x0f ) + ( hi & 0xf0 );
  }
  else
  {
    int sum = a - value - carry;
    v.clear();
    c.clear();
    if ( ( a ^ value ) & ( a ^ sum ) & 0x80 )
    {
      v.set();
    }
    if ( ( sum & 0xff00 ) == 0 )
    {
      c.set();
    }
    a = (uint8_t)sum;
  }
  setnz( a );
}

void CPUState::bit( uint8_t value )
{
  setz( a & value );
  n.set( ( value & 0x80 ) != 0x00 );
  v.set( ( value & 0x40 ) != 0x00 );
}

void CPUState::cmp( uint8_t value )
{
  c.set( a >= value );
  setnz( a - value );;
}

void CPUState::cpx( uint8_t value )
{
  c.set( x >= value );
  setnz( x - value );
}

void CPUState::cpy( uint8_t value )
{
  c.set( y >= value );
  setnz( y - value );
}


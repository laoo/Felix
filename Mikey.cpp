#include "Mikey.hpp"
#include <cassert>

Mikey::Mikey() : mRegs{}
{
}

uint64_t Mikey::requestAccess( uint64_t tick, uint16_t address )
{
  return uint64_t();
}

uint8_t Mikey::read( uint16_t address )
{
  switch ( address )
  {
  default:
    assert( false );
  }

  return uint8_t();
}

void Mikey::write( uint16_t address, uint8_t value )
{
  switch ( address )
  {
  case IODAT:
    mRegs.iodat = value;
    break;
  case IODIR:
    mRegs.iodir = value;
  default:
    assert( false );
  }
}

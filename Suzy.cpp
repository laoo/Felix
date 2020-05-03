#include "Suzy.hpp"
#include <cassert>

Suzy::Suzy()
{
}

uint64_t Suzy::requestAccess( uint64_t tick, uint16_t address )
{
  return tick + 5;
}

uint8_t Suzy::read( uint16_t address )
{
  switch ( address )
  {
  case SUZYHREV:
    return 0x01;
  default:
    assert( false );
  }
  return uint8_t();
}

void Suzy::write( uint16_t address, uint8_t value )
{
  switch ( address )
  {
  default:
    assert( false );
  }
}

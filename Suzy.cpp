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
  address &= 0xff;

  switch ( address )
  {
  case SUZYHREV:
    return 0x01;
  default:
    assert( false );
  }
  return uint8_t();
}

SequencedAction Suzy::write( uint16_t address, uint8_t value )
{
  address &= 0xff;

  switch ( address )
  {
  case HOFF:
  case VOFF:
  case COLLOFF:
  case HSIZOFF:
  case VSIZOFF:
  case SPRINIT:
  case SUZYBUSEN:
  case SPRSYS:
    break;
  default:
    assert( false );
    break;
  }

  return {};
}

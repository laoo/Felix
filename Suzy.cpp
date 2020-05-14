#include "Suzy.hpp"
#include <cassert>

Suzy::Suzy() : mEngine{}
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
  case TMPADR:
    break;
  case TMPADR + 1:
    break;
  case TILTACUM:
    break;
  case TILTACUM + 1:
    break;
  case HOFF:
    break;
  case HOFF + 1:
    break;
  case VOFF:
    break;
  case VOFF + 1:
    break;
  case VIDBAS:
    break;
  case VIDBAS + 1:
    break;
  case COLLBAS:
    break;
  case COLLBAS + 1:
    break;
  case VIDADR:
    break;
  case VIDADR + 1:
    break;
  case COLLADR:
    break;
  case COLLADR + 1:
    break;
  case SCBNEXT:
    break;
  case SCBNEXT + 1:
    break;
  case SPRDLINE:
    break;
  case SPRDLINE + 1:
    break;
  case HPOSSTRT:
    break;
  case HPOSSTRT + 1:
    break;
  case VPOSSTRT:
    break;
  case VPOSSTRT + 1:
    break;
  case SPRHSIZ:
    break;
  case SPRHSIZ + 1:
    break;
  case SPRVSIZ:
    break;
  case SPRVSIZ + 1:
    break;
  case STRETCH:
    break;
  case STRETCH + 1:
    break;
  case TILT:
    break;
  case TILT + 1:
    break;
  case SPRDOFF:
    break;
  case SPRDOFF + 1:
    break;
  case SCVPOS:
    break;
  case SCVPOS + 1:
    break;
  case COLLOFF:
    break;
  case COLLOFF + 1:
    break;
  case VSIZACUM:
    break;
  case VSIZACUM + 1:
    break;
  case HSIZOFF:
    break;
  case HSIZOFF + 1:
    break;
  case VSIZOFF:
    break;
  case VSIZOFF + 1:
    break;
  case SCBADR:
    break;
  case SCBADR + 1:
    break;
  case PROCADR:
    break;
  case PROCADR + 1:
    break;

  case MATHD:
    break;
  case MATHC:
    break;
  case MATHB:
    break;
  case MATHA:
    break;
  case MATHP:
    break;
  case MATHN:

  case MATHH:
    break;
  case MATHG:
    break;
  case MATHF:
    break;
  case MATHE:
    break;

  case MATHM:
    break;
  case MATHL:
    break;
  case MATHK:
    break;
  case MATHJ:
    break;

  case SPRINIT:
    break;
  case SUZYHREV:
    return 0x01;
  case SUZYBUSEN:
    break;
  case SPRSYS:
    break;

  default:
    assert( false );
    break;
  }

  return uint8_t( 0xff );
}

SequencedAction Suzy::write( uint16_t address, uint8_t value )
{
  address &= 0xff;

  switch ( address )
  {
    case TMPADR:
      mEngine.tmpadr = value;
      break;
    case TMPADR + 1:
      mEngine.tmpadr.h = value;
      break;
    case TILTACUM:
      mEngine.tiltacum = value;
      break;
    case TILTACUM + 1:
      break;
      mEngine.tiltacum.h = value;
    case HOFF:
      mEngine.hoff = value;
      break;
    case HOFF + 1:
      mEngine.hoff.h = value;
      break;
    case VOFF:
      mEngine.voff = value;
      break;
    case VOFF + 1:
      mEngine.voff.h = value;
      break;
    case VIDBAS:
      mEngine.vidbas = value;
      break;
    case VIDBAS + 1:
      mEngine.vidbas.h = value;
      break;
    case COLLBAS:
      mEngine.collbas = value;
      break;
    case COLLBAS + 1:
      mEngine.collbas.h = value;
      break;
    case VIDADR:
      mEngine.vidadr = value;
      break;
    case VIDADR + 1:
      mEngine.vidadr.h = value;
      break;
    case COLLADR:
      mEngine.colladr = value;
      break;
    case COLLADR + 1:
      mEngine.colladr.h = value;
      break;
    case SCBNEXT:
      mEngine.scbnext = value;
      break;
    case SCBNEXT + 1:
      mEngine.scbnext.h = value;
      break;
    case SPRDLINE:
      mEngine.sprdline = value;
      break;
    case SPRDLINE + 1:
      mEngine.sprdline.h = value;
      break;
    case HPOSSTRT:
      mEngine.hposstrt = value;
      break;
    case HPOSSTRT + 1:
      mEngine.hposstrt.h = value;
      break;
    case VPOSSTRT:
      mEngine.vposstrt = value;
      break;
    case VPOSSTRT + 1:
      mEngine.vposstrt.h = value;
      break;
    case SPRHSIZ:
      mEngine.sprhsiz = value;
      break;
    case SPRHSIZ + 1:
      mEngine.sprhsiz.h = value;
      break;
    case SPRVSIZ:
      mEngine.sprvsiz = value;
      break;
    case SPRVSIZ + 1:
      mEngine.sprvsiz.h = value;
      break;
    case STRETCH:
      mEngine.stretch = value;
      break;
    case STRETCH + 1:
      mEngine.stretch.h = value;
      break;
    case TILT:
      mEngine.tilt = value;
      break;
    case TILT + 1:
      mEngine.tilt.h = value;
      break;
    case SPRDOFF:
      mEngine.sprdoff = value;
      break;
    case SPRDOFF + 1:
      mEngine.sprdoff.h = value;
      break;
    case SCVPOS:
      mEngine.scvpos = value;
      break;
    case SCVPOS + 1:
      mEngine.scvpos.h = value;
      break;
    case COLLOFF:
      mEngine.colloff = value;
      break;
    case COLLOFF + 1:
      mEngine.colloff.h = value;
      break;
    case VSIZACUM:
      mEngine.vsizacum = value;
      break;
    case VSIZACUM + 1:
      mEngine.vsizacum.h = value;
      break;
    case HSIZOFF:
      mEngine.hsizoff = value;
      break;
    case HSIZOFF + 1:
      mEngine.hsizoff.h = value;
      break;
    case VSIZOFF:
      mEngine.vsizoff = value;
      break;
    case VSIZOFF + 1:
      mEngine.vsizoff.h = value;
      break;
    case SCBADR:
      mEngine.scbadr = value;
      break;
    case SCBADR + 1:
      mEngine.scbadr.h = value;
      break;
    case PROCADR:
      mEngine.procadr = value;
      break;
    case PROCADR + 1:
      mEngine.procadr.h = value;
      break;

    case MATHD:
      mEngine.mathd = value;
      break;
    case MATHC:
      mEngine.mathc = value;
      break;
    case MATHB:
      mEngine.mathb = value;
      break;
    case MATHA:
      mEngine.matha = value;
      break;
    case MATHP:
      mEngine.mathp = value;
      break;
    case MATHN:
      mEngine.mathn = value;
      break;

    case MATHH:
      mEngine.mathh = value;
      break;
    case MATHG:
      mEngine.mathg = value;
      break;
    case MATHF:
      mEngine.mathf = value;
      break;
    case MATHE:
      mEngine.mathe = value;
      break;

    case MATHM:
      mEngine.mathm = value;
      break;
    case MATHL:
      mEngine.mathl = value;
      break;
    case MATHK:
      mEngine.mathk = value;
      break;
    case MATHJ:
      mEngine.mathj = value;
      break;

    case SPRINIT:
      //break;
    case SUZYHREV:
      //break;
    case SUZYBUSEN:
      mBusEnable = ( SUZYBUSEN::ENABLE & value ) != 0;
      break;
    case SPRSYS:
      //break;

    default:
    assert( false );
    break;
  }

  return {};
}

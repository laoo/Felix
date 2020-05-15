#include "Suzy.hpp"
#include <cassert>

Suzy::Suzy() : mEngine{},
  mBusEnable{}, mSignMath{}, mAccumulate{}, mNoCollide{}, mVStretch{}, mLeftHand{}, mUnsafeAccess{}, mSpriteStop{}, mMathWorking{},
  mMathWarning{}, mMathCarry{}, mSpriteWorking{}, mHFlip{}, mVFlip{}, mLiteral{}, mAlgo3{}, mReusePalette{}, mSkipSprite{}, mDrawUp{}, mDrawLeft{}, mEveron{}, mBpp{}, mSpriteType{}, mReload{},
  mSprColl{}, mSprInit{}, mJoystick{}, mSwitches{}, mCart0{}, mCart1{}
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
    return mEngine.tmpadr.l;
  case TMPADR + 1:
    return mEngine.tmpadr.h;
  case TILTACUM:
    return mEngine.tiltacum.l;
  case TILTACUM + 1:
    return mEngine.tiltacum.h;
  case HOFF:
    return mEngine.hoff.l;
  case HOFF + 1:
    return mEngine.hoff.h;
  case VOFF:
    return mEngine.voff.l;
  case VOFF + 1:
    return mEngine.hoff.h;
  case VIDBAS:
    return mEngine.vidbas.l;
  case VIDBAS + 1:
    return mEngine.vidbas.h;
  case COLLBAS:
    return mEngine.collbas.l;
  case COLLBAS + 1:
    return mEngine.collbas.h;
  case VIDADR:
    return mEngine.vidadr.l;
  case VIDADR + 1:
    return mEngine.vidadr.h;
  case COLLADR:
    return mEngine.colladr.l;
  case COLLADR + 1:
    return mEngine.colladr.h;
  case SCBNEXT:
    return mEngine.scbnext.l;
  case SCBNEXT + 1:
    return mEngine.scbnext.h;
  case SPRDLINE:
    return mEngine.sprdline.l;
  case SPRDLINE + 1:
    return mEngine.sprdline.h;
  case HPOSSTRT:
    return mEngine.hposstrt.l;
  case HPOSSTRT + 1:
    return mEngine.hposstrt.h;
  case VPOSSTRT:
    return mEngine.vposstrt.l;
  case VPOSSTRT + 1:
    return mEngine.vposstrt.h;
  case SPRHSIZ:
    return mEngine.sprhsiz.l;
  case SPRHSIZ + 1:
    return mEngine.sprhsiz.h;
  case SPRVSIZ:
    return mEngine.sprvsiz.l;
  case SPRVSIZ + 1:
    return mEngine.sprvsiz.h;
  case STRETCH:
    return mEngine.stretch.l;
  case STRETCH + 1:
    return mEngine.stretch.h;
  case TILT:
    return mEngine.tilt.l;
  case TILT + 1:
    return mEngine.tilt.h;
  case SPRDOFF:
    return mEngine.sprdoff.l;
  case SPRDOFF + 1:
    return mEngine.sprdoff.h;
  case SCVPOS:
    return mEngine.scvpos.l;
  case SCVPOS + 1:
    return mEngine.scvpos.h;
  case COLLOFF:
    return mEngine.colloff.l;
  case COLLOFF + 1:
    return mEngine.colloff.h;
  case VSIZACUM:
    return mEngine.vsizacum.l;
  case VSIZACUM + 1:
    return mEngine.vsizacum.h;
  case HSIZOFF:
    return mEngine.hsizoff.l;
  case HSIZOFF + 1:
    return mEngine.hsizoff.h;
  case VSIZOFF:
    return mEngine.vsizoff.l;
  case VSIZOFF + 1:
    return mEngine.vsizoff.h;
  case SCBADR:
    return mEngine.scbadr.l;
  case SCBADR + 1:
    return mEngine.scbadr.h;
  case PROCADR:
    return mEngine.procadr.l;
  case PROCADR + 1:
    return mEngine.procadr.h;
  case MATHD:
    return mEngine.mathd;
  case MATHC:
    return mEngine.mathc;
  case MATHB:
    return mEngine.mathb;
  case MATHA:
    return mEngine.matha;
  case MATHP:
    return mEngine.mathp;
  case MATHN:
    return mEngine.mathn;
  case MATHH:
    return mEngine.mathh;
  case MATHG:
    return mEngine.mathg;
  case MATHF:
    return mEngine.mathf;
  case MATHE:
    return mEngine.mathe;
  case MATHM:
    return mEngine.mathm;
  case MATHL:
    return mEngine.mathl;
  case MATHK:
    return mEngine.mathk;
  case MATHJ:
    return mEngine.mathj;
  case SUZYHREV:
    return 0x01;
  case SPRSYS:
    return
      ( mMathWorking ? SPRSYS::MATHWORKING : 0 ) |
      ( mMathWarning ? SPRSYS::MATHWARNING : 0 ) |
      ( mMathCarry ? SPRSYS::MATHCARRY : 0 ) |
      ( mVStretch ? SPRSYS::VSTRETCHING : 0 ) |
      ( mLeftHand ? SPRSYS::LEFTHANDED : 0 ) |
      ( mUnsafeAccess ? SPRSYS::UNSAFEACCESS : 0 ) |
      ( mSpriteStop ? SPRSYS::SPRITETOSTOP : 0 ) |
      ( mSpriteWorking ? SPRSYS::SPRITEWORKING : 0 );
    break;
  case JOYSTICK:
    return mJoystick;
  case SWITCHES:
    return mSwitches;
  case RCART0:
    return mCart0;
  case RCART1:
    return mCart1;
  default:
    return uint8_t( 0xff );
  }
}

void Suzy::write( uint16_t address, uint8_t value )
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
      mEngine.tiltacum.h = value;
      break;
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

    case SPRCTL0:
      writeSPRCTL0( value );
      break;
    case SPRCTL1:
      writeSPRCTL1( value );
      break;
    case SPRCOLL:
      mSprColl = value;
    case SPRINIT:
      mSprInit = value;
      break;
    case SUZYBUSEN:
      mBusEnable = ( SUZYBUSEN::ENABLE & value ) != 0;
      break;
    case SPRGO:
      mSpriteStop = false;
      mEveron = ( SPRGO::EVER_ON & value ) != 0;
      mSpriteWorking = ( SPRGO::SPRITE_GO & value ) != 0;
      break;
    case SPRSYS:
      mSignMath = ( SPRSYS::SIGNMATH & value ) != 0;
      mAccumulate = ( SPRSYS::ACCUMULATE & value ) != 0;
      mNoCollide = ( SPRSYS::NO_COLLIDE & value ) != 0;
      mVStretch = ( SPRSYS::VSTRETCH & value ) != 0;
      mLeftHand = ( SPRSYS::LEFTHAND & value ) != 0;
      mUnsafeAccess = mUnsafeAccess && ( SPRSYS::UNSAFEACCESSRST & value ) == 0;
      mSpriteStop = ( SPRSYS::SPRITESTOP & value ) != 0;
      break;
    case RCART0:
      writeCart( 0, value );
      break;
    case RCART1:
      writeCart( 1, value );
      break;
    default:
    assert( false );
    break;
  }
}


void Suzy::writeSPRCTL0( uint8_t value )
{
  mBpp = (BPP)( value & SPRCTL0::BITS_MASK );
  mHFlip = ( value & SPRCTL0::HFLIP ) != 0;
  mVFlip = ( value & SPRCTL0::VFLIP ) != 0;
  mSpriteType = (Sprite)( value & SPRCTL0::SPRITE_MASK );
}

void Suzy::writeSPRCTL1( uint8_t value )
{
  mLiteral = ( value & SPRCTL1::LITERAL ) != 0;
  mAlgo3 = ( value & SPRCTL1::ALGO_3 ) != 0;
  mReload = (Reload)( value & SPRCTL1::RELOAD_MASK );
  mReusePalette = ( value & SPRCTL1::REUSE_PALETTE ) != 0;
  mSkipSprite = ( value & SPRCTL1::SKIP_SPRITE ) != 0;
  mDrawUp = ( value & SPRCTL1::DRAW_UP ) != 0;
  mDrawLeft = ( value & SPRCTL1::DRAW_LEFT ) != 0;
}

void Suzy::writeCart( int number, uint8_t value )
{
}

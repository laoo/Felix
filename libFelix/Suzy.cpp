#include "pch.hpp"
#include "Suzy.hpp"
#include "Core.hpp"
#include "SuzyProcess.hpp"
#include "Cartridge.hpp"
#include "Log.hpp"

Suzy::Suzy( Core & core, std::shared_ptr<IInputSource> inputSource ) : mCore{ core }, mSCB{}, mMath{ mCore.getTraceHelper() }, mInputSource{ inputSource }, mAccessTick{},
  mPalette{}, mBusEnable{}, mNoCollide{}, mVStretch{}, mLeftHand{ true }, mUnsafeAccess{}, mSpriteStop{},
  mSpriteWorking{}, mHFlip{}, mVFlip{}, mLiteral{}, mAlgo3{}, mReusePalette{}, mSkipSprite{}, mStartingQuadrant{}, mEveron{},
  mBpp{}, mSpriteType{}, mReload{}, mSprColl{}, mSprInit{}
{
}

uint64_t Suzy::requestRead( uint64_t tick, uint16_t address )
{
  address &= 0xff;

  switch ( address )
  {
  case RCART0:
  case RCART1:
    mAccessTick = tick + 14;
    break;
  default:
    mAccessTick = tick + 5;
  }

  return mAccessTick;
}

uint64_t Suzy::requestWrite( uint64_t tick, uint16_t address )
{
  address &= 0xff;

  switch ( address )
  {
  case RCART0:
  case RCART1:
    mAccessTick = tick + 14;
    break;
  default:
    mAccessTick = tick + 9;
  }

  return mAccessTick;
}

uint8_t Suzy::read( uint16_t address )
{
  address &= 0xff;

  switch ( address )
  {
  case TMPADR:
    return mSCB.tmpadr.l;
  case TMPADR + 1:
    return mSCB.tmpadr.h;
  case TILTACUM:
    return mSCB.tiltacum.l;
  case TILTACUM + 1:
    return mSCB.tiltacum.h;
  case HOFF:
    return mSCB.hoff.l;
  case HOFF + 1:
    return mSCB.hoff.h;
  case VOFF:
    return mSCB.voff.l;
  case VOFF + 1:
    return mSCB.voff.h;
  case VIDBAS:
    return mSCB.vidbas.l;
  case VIDBAS + 1:
    return mSCB.vidbas.h;
  case COLLBAS:
    return mSCB.collbas.l;
  case COLLBAS + 1:
    return mSCB.collbas.h;
  case VIDADR:
    return mSCB.vidadr.l;
  case VIDADR + 1:
    return mSCB.vidadr.h;
  case COLLADR:
    return mSCB.colladr.l;
  case COLLADR + 1:
    return mSCB.colladr.h;
  case SCBNEXT:
    return mSCB.scbnext.l;
  case SCBNEXT + 1:
    return mSCB.scbnext.h;
  case SPRDLINE:
    return mSCB.sprdline.l;
  case SPRDLINE + 1:
    return mSCB.sprdline.h;
  case HPOSSTRT:
    return mSCB.hposstrt.l;
  case HPOSSTRT + 1:
    return mSCB.hposstrt.h;
  case VPOSSTRT:
    return mSCB.vposstrt.l;
  case VPOSSTRT + 1:
    return mSCB.vposstrt.h;
  case SPRHSIZ:
    return mSCB.sprhsiz.l;
  case SPRHSIZ + 1:
    return mSCB.sprhsiz.h;
  case SPRVSIZ:
    return mSCB.sprvsiz.l;
  case SPRVSIZ + 1:
    return mSCB.sprvsiz.h;
  case STRETCH:
    return mSCB.stretch.l;
  case STRETCH + 1:
    return mSCB.stretch.h;
  case TILT:
    return mSCB.tilt.l;
  case TILT + 1:
    return mSCB.tilt.h;
  case SPRDOFF:
    return mSCB.sprdoff.l;
  case SPRDOFF + 1:
    return mSCB.sprdoff.h;
  case SCVPOS:
    return mSCB.sprvpos.l;
  case SCVPOS + 1:
    return mSCB.sprvpos.h;
  case COLLOFF:
    return mSCB.colloff.l;
  case COLLOFF + 1:
    return mSCB.colloff.h;
  case VSIZACUM:
    return mSCB.vsizacum.l;
  case VSIZACUM + 1:
    return mSCB.vsizacum.h;
  case HSIZOFF:
    return mSCB.hsizoff.l;
  case HSIZOFF + 1:
    return mSCB.hsizoff.h;
  case VSIZOFF:
    return mSCB.vsizoff.l;
  case VSIZOFF + 1:
    return mSCB.vsizoff.h;
  case SCBADR:
    return mSCB.scbadr.l;
  case SCBADR + 1:
    return mSCB.scbadr.h;
  case PROCADR:
    return mSCB.procadr.l;
  case PROCADR + 1:
    return mSCB.procadr.h;
  case MATHD:
  case MATHC:
  case MATHB:
  case MATHA:
  case MATHP:
  case MATHN:
  case MATHH:
  case MATHG:
  case MATHF:
  case MATHE:
  case MATHM:
  case MATHL:
  case MATHK:
  case MATHJ:
    return mMath.peek( mAccessTick, address & 0xff );
  case SUZYHREV:
    return 0x01;
  case SPRSYS:
    return
      ( mMath.working( mAccessTick ) ? SPRSYS::MATHWORKING : 0 ) |
      ( mMath.warning() ? SPRSYS::MATHWARNING : 0 ) |
      ( mMath.carry() ? SPRSYS::MATHCARRY : 0 ) |
      ( mVStretch ? SPRSYS::VSTRETCHING : 0 ) |
      ( mLeftHand ? SPRSYS::LEFTHANDED : 0 ) |
      ( mMath.unsafeAccess() ? SPRSYS::UNSAFEACCESS : 0 ) |
      ( mSpriteStop ? SPRSYS::SPRITETOSTOP : 0 ) |
      ( mSpriteWorking ? SPRSYS::SPRITEWORKING : 0 );
    break;
  case JOYSTICK:
  {
    uint8_t joystick = mInputSource->getInput( mLeftHand != 0 ).joystick();
    return joystick;
  }
  case SWITCHES:
  {
    uint8_t switches = mInputSource->getInput( mLeftHand != 0 ).switches() |
      ( mCore.getCartridge().isCart0Inactive() ? SWITCHES::CART0_STROBE : 0 ) |
      ( mCore.getCartridge().isCart1Inactive() ? SWITCHES::CART1_STROBE : 0 );
    return switches;
  }
  case RCART0:
    return mCore.getCartridge().peekRCART0( mAccessTick );
  case RCART1:
  {
    //incrementing counter...
    mCore.getCartridge().peekRCART1( mAccessTick );
    //... but looks like mirror of joystick
    return mInputSource->getInput( mLeftHand != 0 ).joystick();
  }
  default:
    if ( address < 0x80 )
    {
      //reading these registers looks like being mirrored in the rane $fc00-$fc40. Needs more investigation
      return 0;
    }
    else
    {
      //undefined registers in range $fc80-$fcff are noicy
      return noice( mAccessTick );
    }
  }
}

void Suzy::write( uint16_t address, uint8_t value )
{
  address &= 0xff;

  switch ( address )
  {
    case TMPADR:
      mSCB.tmpadr = value;
      break;
    case TMPADR + 1:
      mSCB.tmpadr.h = value;
      break;
    case TILTACUM:
      mSCB.tiltacum = value;
      break;
    case TILTACUM + 1:
      mSCB.tiltacum.h = value;
      break;
    case HOFF:
      mSCB.hoff = value;
      break;
    case HOFF + 1:
      mSCB.hoff.h = value;
      break;
    case VOFF:
      mSCB.voff = value;
      break;
    case VOFF + 1:
      mSCB.voff.h = value;
      break;
    case VIDBAS:
      mSCB.vidbas = value;
      break;
    case VIDBAS + 1:
      mSCB.vidbas.h = value;
      break;
    case COLLBAS:
      mSCB.collbas = value;
      break;
    case COLLBAS + 1:
      mSCB.collbas.h = value;
      break;
    case VIDADR:
      mSCB.vidadr = value;
      break;
    case VIDADR + 1:
      mSCB.vidadr.h = value;
      break;
    case COLLADR:
      mSCB.colladr = value;
      break;
    case COLLADR + 1:
      mSCB.colladr.h = value;
      break;
    case SCBNEXT:
      mSCB.scbnext = value;
      break;
    case SCBNEXT + 1:
      mSCB.scbnext.h = value;
      break;
    case SPRDLINE:
      mSCB.sprdline = value;
      break;
    case SPRDLINE + 1:
      mSCB.sprdline.h = value;
      break;
    case HPOSSTRT:
      mSCB.hposstrt = value;
      break;
    case HPOSSTRT + 1:
      mSCB.hposstrt.h = value;
      break;
    case VPOSSTRT:
      mSCB.vposstrt = value;
      break;
    case VPOSSTRT + 1:
      mSCB.vposstrt.h = value;
      break;
    case SPRHSIZ:
      mSCB.sprhsiz = value;
      break;
    case SPRHSIZ + 1:
      mSCB.sprhsiz.h = value;
      break;
    case SPRVSIZ:
      mSCB.sprvsiz = value;
      break;
    case SPRVSIZ + 1:
      mSCB.sprvsiz.h = value;
      break;
    case STRETCH:
      mSCB.stretch = value;
      break;
    case STRETCH + 1:
      mSCB.stretch.h = value;
      break;
    case TILT:
      mSCB.tilt = value;
      break;
    case TILT + 1:
      mSCB.tilt.h = value;
      break;
    case SPRDOFF:
      mSCB.sprdoff = value;
      break;
    case SPRDOFF + 1:
      mSCB.sprdoff.h = value;
      break;
    case SCVPOS:
      mSCB.sprvpos = value;
      break;
    case SCVPOS + 1:
      mSCB.sprvpos.h = value;
      break;
    case COLLOFF:
      mSCB.colloff = value;
      break;
    case COLLOFF + 1:
      mSCB.colloff.h = value;
      break;
    case VSIZACUM:
      mSCB.vsizacum = value;
      break;
    case VSIZACUM + 1:
      mSCB.vsizacum.h = value;
      break;
    case HSIZOFF:
      mSCB.hsizoff = value;
      break;
    case HSIZOFF + 1:
      mSCB.hsizoff.h = value;
      break;
    case VSIZOFF:
      mSCB.vsizoff = value;
      break;
    case VSIZOFF + 1:
      mSCB.vsizoff.h = value;
      break;
    case SCBADR:
      mSCB.scbadr = value;
      break;
    case SCBADR + 1:
      mSCB.scbadr.h = value;
      break;
    case PROCADR:
      mSCB.procadr = value;
      break;
    case PROCADR + 1:
      mSCB.procadr.h = value;
      break;

    case MATHM:
      mMath.carry( false );
      [[fallthrough]];
    case MATHD:
    case MATHB:
    case MATHP:
    case MATHH:
    case MATHF:
    case MATHK:
      mMath.wpoke( mAccessTick, address & 0xff, value );
      break;
    case MATHC:
      mMath.poke( mAccessTick, address & 0xff, value );
      mMath.signCD();
      break;
    case MATHA:
      if ( mMath.poke( mAccessTick, address & 0xff, value ) )
      {
        mMath.signAB();
        mMath.mul( mAccessTick );
      }
      break;
    case MATHE:
      if ( mMath.poke( mAccessTick, address & 0xff, value ) )
      {
        mMath.div( mAccessTick );
      }
      break;
    case MATHN:
    case MATHG:
    case MATHL:
    case MATHJ:
      mMath.poke( mAccessTick, address & 0xff, value );
      break;
    case SPRCTL0:
      writeSPRCTL0( value );
      break;
    case SPRCTL1:
      writeSPRCTL1( value );
      break;
    case SPRCOLL:
      mSprColl = value;
      break;
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
      mMath.signMath( ( SPRSYS::SIGNMATH & value ) != 0 );
      mMath.accumulate( ( SPRSYS::ACCUMULATE & value ) != 0 );
      mNoCollide = ( SPRSYS::NO_COLLIDE & value ) != 0;
      mVStretch = ( SPRSYS::VSTRETCH & value ) != 0;
      mLeftHand = ( SPRSYS::LEFTHAND & value ) != 0;
      mMath.unsafeAccess( mMath.unsafeAccess() && ( SPRSYS::UNSAFEACCESSRST & value ) == 0 );
      mSpriteStop = ( SPRSYS::SPRITESTOP & value ) != 0;
      break;
    case RCART0:
      mCore.getCartridge().pokeRCART0( mAccessTick, value );
      break;
    case RCART1:
      mCore.getCartridge().pokeRCART1( mAccessTick, value );
      break;
    default:
    assert( false );
    break;
  }
}

uint16_t Suzy::debugVidBas() const
{
  return mSCB.vidbas;
}

uint16_t Suzy::debugCollBas() const
{
  return mSCB.collbas;
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
  mStartingQuadrant = ( Quadrant )( value & SPRCTL1::STARGING_QUAD_MASK );
}

void Suzy::writeSPRCOLL( uint8_t value )
{
  mSprColl = value;
}

int Suzy::bpp() const
{
  return ( ( int )mBpp >> 6 ) + 1;
}

uint8_t Suzy::noice( uint64_t tick )
{
  //undefined registers (with address > $FC80) has some peculiar random noice characteristics that looks something like this
  auto v = std::hash<uint64_t>()( tick ) & 0xffff;
  if ( v < 700 )
  {
    return v & 1 ? 0 : 0xff;
  }
  else
  {
    return 0b11111100;
  }
}

std::shared_ptr<ISuzyProcess> Suzy::suzyProcess()
{
  return std::make_shared<SuzyProcess>( *this );
}

#include "Suzy.hpp"
#include <cassert>
#include "BusMaster.hpp"

Suzy::Suzy() : mSCB{}, mMath{},
  mBusEnable{}, mSignMath{}, mAccumulate{}, mNoCollide{}, mVStretch{}, mLeftHand{}, mUnsafeAccess{}, mSpriteStop{}, mMathWorking{},
  mMathWarning{}, mMathCarry{}, mSpriteWorking{}, mHFlip{}, mVFlip{}, mLiteral{}, mAlgo3{}, mReusePalette{}, mSkipSprite{}, mStartingQuadrant{}, mEveron{}, mFred{},
  mBpp{}, mSpriteType{}, mReload{}, mSprColl{}, mSprInit{}, mJoystick{}, mSwitches{}, mCart0{}, mCart1{}
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
    return mSCB.hoff.h;
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
    return mMath.mathd;
  case MATHC:
    return mMath.mathc;
  case MATHB:
    return mMath.mathb;
  case MATHA:
    return mMath.matha;
  case MATHP:
    return mMath.mathp;
  case MATHN:
    return mMath.mathn;
  case MATHH:
    return mMath.mathh;
  case MATHG:
    return mMath.mathg;
  case MATHF:
    return mMath.mathf;
  case MATHE:
    return mMath.mathe;
  case MATHM:
    return mMath.mathm;
  case MATHL:
    return mMath.mathl;
  case MATHK:
    return mMath.mathk;
  case MATHJ:
    return mMath.mathj;
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

    case MATHD:
      mMath.mathd = value;
      break;
    case MATHC:
      mMath.mathc = value;
      break;
    case MATHB:
      mMath.mathb = value;
      break;
    case MATHA:
      mMath.matha = value;
      break;
    case MATHP:
      mMath.mathp = value;
      break;
    case MATHN:
      mMath.mathn = value;
      break;

    case MATHH:
      mMath.mathh = value;
      break;
    case MATHG:
      mMath.mathg = value;
      break;
    case MATHF:
      mMath.mathf = value;
      break;
    case MATHE:
      mMath.mathe = value;
      break;

    case MATHM:
      mMath.mathm = value;
      break;
    case MATHL:
      mMath.mathl = value;
      break;
    case MATHK:
      mMath.mathk = value;
      break;
    case MATHJ:
      mMath.mathj = value;
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
  mStartingQuadrant = ( Quadrant )( value & SPRCTL1::STARGING_QUAD_MASK );
}

void Suzy::writeCart( int number, uint8_t value )
{
}

int Suzy::bpp() const
{
  return ( ( int )mBpp >> 6 ) + 1;
}

SuzyCoSubroutine Suzy::loadSCB( SuzyRequest & req )
{
  co_await req;

  writeSPRCTL0( co_await SuzyRead{ mSCB.scbadr++ } );
  writeSPRCTL1( co_await SuzyRead{ mSCB.scbadr++ } );
  mSprColl = co_await SuzyRead{ mSCB.scbadr++ };
  mSCB.scbnext.l = co_await SuzyRead{ mSCB.scbadr++ };
  mSCB.scbnext.h = co_await SuzyRead{ mSCB.scbadr++ };

  if ( mSkipSprite )
    co_return;

  mSCB.sprdline.l = co_await SuzyRead{ mSCB.scbadr++ };
  mSCB.sprdline.h = co_await SuzyRead{ mSCB.scbadr++ };
  mSCB.hposstrt.l = co_await SuzyRead{ mSCB.scbadr++ };
  mSCB.hposstrt.h = co_await SuzyRead{ mSCB.scbadr++ };
  mSCB.vposstrt.l = co_await SuzyRead{ mSCB.scbadr++ };
  mSCB.vposstrt.h = co_await SuzyRead{ mSCB.scbadr++ };

  mSCB.tilt = 0;
  mSCB.stretch = 0;

  switch ( mReload )
  {
  case Reload::HVST:  //Reload hsize, vsize, stretch, tilt
    mSCB.sprhsiz.l = co_await SuzyRead{ mSCB.scbadr++ };
    mSCB.sprhsiz.h = co_await SuzyRead{ mSCB.scbadr++ };
    mSCB.sprvsiz.l = co_await SuzyRead{ mSCB.scbadr++ };
    mSCB.sprvsiz.h = co_await SuzyRead{ mSCB.scbadr++ };
    mSCB.stretch.l = co_await SuzyRead{ mSCB.scbadr++ };
    mSCB.stretch.h = co_await SuzyRead{ mSCB.scbadr++ };
    mSCB.tilt.l = co_await SuzyRead{ mSCB.scbadr++ };
    mSCB.tilt.h = co_await SuzyRead{ mSCB.scbadr++ };
    break;
  case Reload::HVS:   //Reload hsize, vsize, stretch
    mSCB.sprhsiz.l = co_await SuzyRead{ mSCB.scbadr++ };
    mSCB.sprhsiz.h = co_await SuzyRead{ mSCB.scbadr++ };
    mSCB.sprvsiz.l = co_await SuzyRead{ mSCB.scbadr++ };
    mSCB.sprvsiz.h = co_await SuzyRead{ mSCB.scbadr++ };
    mSCB.stretch.l = co_await SuzyRead{ mSCB.scbadr++ };
    mSCB.stretch.h = co_await SuzyRead{ mSCB.scbadr++ };
    break;
  case Reload::HV:    //Reload hsize, vsize
    mSCB.sprhsiz.l = co_await SuzyRead{ mSCB.scbadr++ };
    mSCB.sprhsiz.h = co_await SuzyRead{ mSCB.scbadr++ };
    mSCB.sprvsiz.l = co_await SuzyRead{ mSCB.scbadr++ };
    mSCB.sprvsiz.h = co_await SuzyRead{ mSCB.scbadr++ };
    break;
  case Reload::NONE:  //Reload nothing
    break;
  }

  if ( !mReusePalette )
  {
    uint32_t p0 = co_await SuzyRead4{ mSCB.scbadr };
    mSCB.scbadr += 4;
    uint32_t p1 = co_await SuzyRead4{ mSCB.scbadr };
    mSCB.scbadr += 4;

    //TODO: implement bug:
    //The page break signal does not delay the end of the pen index palette loading.
    *( (uint32_t*)mPalette.data() ) = p0;
    *( ( uint32_t* )( mPalette.data() + 4 ) ) = p1;
  }
}


PenUnpacker Suzy::pixelUnpacker()
{
  for co_await( auto & line : PenUnpacker::Sprite{} )
  {
    for co_await ( auto pen : line )
    {
      co_yield pen;
    }
  }
}

SuzyCoSubroutineT<bool> Suzy::renderSingleSprite( SuzyRequest & req )
{
  co_await req;

  if ( mSkipSprite )
    co_return false;

  auto unpacker = pixelUnpacker();

  auto const& quadCycle = mQuadrantOrder[( size_t )mStartingQuadrant];
  size_t quadrant = ~0;
  bool isEveron{};
  int dx{}, dy{};

  for ( auto result = PenUnpacker::Result{ PenUnpacker::Status::NEXT_QUADRANT};; result = unpacker() )
  {
    switch ( result )
    {
    case PenUnpacker::Status::DATA_NEEDED:
      unpacker.feedData( co_await SuzyRead4{ mSCB.procadr } );
      mSCB.procadr += 4;
      break;
    case PenUnpacker::Status::PEN_READY:
      break;
    case PenUnpacker::Status::NEXT_LINE:
      mSCB.sprdoff = unpacker.startLine( bpp(), mLiteral, co_await SuzyRead4{ mSCB.procadr } );
      mSCB.procadr += 4;
      break;
    case PenUnpacker::Status::NEXT_QUADRANT:
      if ( ++quadrant > 3 )
        co_return isEveron;
      dx = ( ( uint8_t )quadCycle[quadrant] & SPRCTL1::DRAW_LEFT ) == 0 ? 1 : -1;
      dy = ( ( uint8_t )quadCycle[quadrant] & SPRCTL1::DRAW_UP ) == 0 ? 1 : -1;
      dx *= mHFlip ? -1 : 1;
      dy *= mVFlip ? -1 : 1;
      mSCB.tiltacum = 0;
      mSCB.vsizacum = ( dy == 1 ) ? mSCB.vsizoff.w : 0;
      mSCB.sprvpos = mSCB.vposstrt - mSCB.voff;
      mSCB.procadr = mSCB.sprdline;
      if ( ( ( uint8_t )quadCycle[quadrant] & SPRCTL1::DRAW_UP ) != ( ( uint8_t )quadCycle[( size_t )mStartingQuadrant] & SPRCTL1::DRAW_UP ) )
        mSCB.sprvpos += dx;
      break;
    case PenUnpacker::Status::NEXT_SPRITE:
      co_return isEveron;
    }
  }
}

SuzySpriteProcessor Suzy::processSprites( SuzyRequest & req )
{
  co_await req;
 
  while ( ( mSCB.scbnext & 0xff00 ) != 0 )
  {
    mSCB.scbadr = mSCB.scbnext;
    mSCB.tmpadr = mSCB.scbadr;

    co_await loadSCB( req );

    mFred = 0;

    bool isEveronScreen = co_await renderSingleSprite( req );

    if ( mEveron && !isEveronScreen )
    {
      co_await SuzyRMW{ ( uint16_t )( mSCB.scbadr + mSCB.colloff ), 0xff, 0x80 };
    }
  }
  
  co_return;
}

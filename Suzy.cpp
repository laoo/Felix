#include "Suzy.hpp"
#include <cassert>
#include "BusMaster.hpp"
#include "SuzyProcess.hpp"

Suzy::Suzy() : mSCB{}, mMath{}, mAccessTick{},
  mBusEnable{}, mNoCollide{}, mVStretch{}, mLeftHand{}, mUnsafeAccess{}, mSpriteStop{},
  mSpriteWorking{}, mHFlip{}, mVFlip{}, mLiteral{}, mAlgo3{}, mReusePalette{}, mSkipSprite{}, mStartingQuadrant{}, mEveron{}, mFred{},
  mBpp{}, mSpriteType{}, mReload{}, mSprColl{}, mSprInit{}, mJoystick{}, mSwitches{}, mCart0{}, mCart1{}
{
}

void Suzy::updateKeyInput( KeyInput const & input )
{
  mJoystick =
    ( input.opt1 ? JOYSTICK::OPTION1 : 0 ) |
    ( input.opt2 ? JOYSTICK::OPTION2 : 0 ) |
    ( input.b ? JOYSTICK::A : 0 ) |
    ( input.a ? JOYSTICK::B : 0 );

  if ( mLeftHand )
  {
    mJoystick |=
      ( input.down ? JOYSTICK::UP : 0 ) |
      ( input.up ? JOYSTICK::DOWN : 0 ) |
      ( input.right ? JOYSTICK::LEFT : 0 ) |
      ( input.left ? JOYSTICK::RIGHT : 0 );
  }
  else
  {
    mJoystick |=
      ( input.down ? JOYSTICK::DOWN : 0 ) |
      ( input.up ? JOYSTICK::UP : 0 ) |
      ( input.right ? JOYSTICK::RIGHT : 0 ) |
      ( input.left ? JOYSTICK::LEFT : 0 );
  }

  mSwitches = input.pause ? SWITCHES::PAUSE_SWITCH : 0;
}

uint64_t Suzy::requestAccess( uint64_t tick, uint16_t address )
{
  mAccessTick = tick + 5;
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
      ( mMath.working() ? SPRSYS::MATHWORKING : 0 ) |
      ( mMath.warning() ? SPRSYS::MATHWARNING : 0 ) |
      ( mMath.carry() ? SPRSYS::MATHCARRY : 0 ) |
      ( mVStretch ? SPRSYS::VSTRETCHING : 0 ) |
      ( mLeftHand ? SPRSYS::LEFTHANDED : 0 ) |
      ( mMath.unsafeAccess() ? SPRSYS::UNSAFEACCESS : 0 ) |
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
    case MATHB:
    case MATHP:
    case MATHH:
    case MATHF:
    case MATHK:
      mMath.wpoke( mAccessTick, address & 0xff, value );
      [[fallthrough]];
    case MATHM:
      mMath.carry( false );
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
    union
    {
      std::array<uint8_t, 8> arr;
      struct
      {
        uint32_t p0;
        uint32_t p1;
      };
    };

    p0 = co_await SuzyRead4{ mSCB.scbadr };
    mSCB.scbadr += 4;
    p1 = co_await SuzyRead4{ mSCB.scbadr };
    mSCB.scbadr += 4;

    //TODO: implement bug:
    //The page break signal does not delay the end of the pen index palette loading.
    for ( size_t i = 0; i < arr.size(); ++i )
    {
      uint8_t value = arr[i];
      mPalette[2 * i] = (uint8_t)( ( value >> 4 ) & 0x0f );
      mPalette[2 * i + 1] = (uint8_t)( value & 0x0f );
    }
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
  int left{}, up{};
  uint16_t hsizacum{};
  int sprhpos{};
  mSCB.vsizacum = mSCB.sprvsiz;
  uint8_t pixelHeight = mSCB.vsizacum.h;
  uint8_t pixelRow = 0;
  mSCB.vsizacum.h = 0;
  mSCB.vidadr = mSCB.vidbas + mSCB.sprvpos * mScreenWidth;
  mSCB.colladr = mSCB.collbas + mSCB.sprvpos * mScreenWidth;
  uint8_t pixelWidth{};
  uint8_t pixel{};
  uint8_t pixelStore{};
  uint8_t pixelMask{};

  for ( auto result = PenUnpacker::Result{ PenUnpacker::Status::NEXT_QUADRANT};; result = unpacker() )
  {
    switch ( result )
    {
    case PenUnpacker::Status::DATA_NEEDED:
      unpacker.feedData( co_await SuzyRead4{ mSCB.procadr } );
      mSCB.procadr += 4;
      break;
    case PenUnpacker::Status::PEN_READY:
      if ( up == 0 && mSCB.sprvpos >= mScreenHeight || up == 1 && (int16_t)( mSCB.sprvpos ) < 0 ) break;

      hsizacum += mSCB.sprhsiz;
      pixelWidth = hsizacum >> 8;
      hsizacum &= 0x00ff;

      assert( result.pixel < 16 );
      pixel = mPalette[result.pixel];

      for ( int h = 0; h < pixelWidth; h++ )
      {
        // Stop horizontal loop if outside of screen bounds
        if ( sprhpos >= 0 && sprhpos < mScreenWidth )
        {
          int even = sprhpos & 1;
          switch ( mSpriteType )
          {
          case Sprite::SHADOW:
            break;
          case Sprite::XOR:
            break;
          case Sprite::NONCOLL:
            break;
          case Sprite::NORMAL:
            break;
          case Sprite::BOUNDARY:
            break;
          case Sprite::BSHADOW:
            break;
          case Sprite::BACKNONCOLL:
            break;
          case Sprite::BACKGROUND:
            break;
          }
          isEveron = true;
        }
        sprhpos += left ? -1 : 1;
      }
      break;
    case PenUnpacker::Status::NEXT_LINE:
      mSCB.procadr = mSCB.sprdline;
      mSCB.sprdoff = unpacker.startLine( bpp(), mLiteral, co_await SuzyRead4{ mSCB.procadr } );
      mSCB.procadr += 4;
      if ( pixelRow++ >= pixelHeight )
      {
        mSCB.sprdline += mSCB.sprdoff;
        mSCB.vsizacum += mSCB.sprvsiz;
        pixelHeight = mSCB.vsizacum.h;
        pixelRow = 0;
        mSCB.vsizacum.h = 0;
        mSCB.vidadr = mSCB.vidbas + mSCB.sprvpos * mScreenWidth / 2;
        mSCB.colladr = mSCB.collbas + mSCB.sprvpos * mScreenWidth / 2;
        mSCB.sprvpos += up ? -1 : 1;
      }
      mSCB.hposstrt += mSCB.tiltacum.h;
      mSCB.tiltacum.h = 0;
      hsizacum = left == 0 ? mSCB.hsizoff.w : 0;
      sprhpos = mSCB.hposstrt - mSCB.hoff;
      break;
    case PenUnpacker::Status::NEXT_QUADRANT:
      if ( ++quadrant > 3 )
        co_return isEveron;
      left = ( ( uint8_t )quadCycle[quadrant] & SPRCTL1::DRAW_LEFT ) == 0 ? 0 : 1;
      up = ( ( uint8_t )quadCycle[quadrant] & SPRCTL1::DRAW_UP ) == 0 ? 0 : 1;
      left ^= mHFlip ? 1 : 0;
      up ^= mVFlip ? 1 : 0;
      mSCB.tiltacum = 0;
      mSCB.vsizacum = ( up == 0 ) ? mSCB.vsizoff.w : 0;
      mSCB.sprvpos = mSCB.vposstrt - mSCB.voff;
      if ( ( ( uint8_t )quadCycle[quadrant] & SPRCTL1::DRAW_UP ) != ( ( uint8_t )quadCycle[( size_t )mStartingQuadrant] & SPRCTL1::DRAW_UP ) )
        mSCB.sprvpos += up ? -1 : 1;
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

std::shared_ptr<ISuzyProcess> Suzy::suzyProcess()
{
  return std::make_shared<SuzyProcess>( *this );
}

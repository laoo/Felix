#include "pch.hpp"
#include "SuzyProcess.hpp"
#include "VidOperator.hpp"
#include "ColOperator.hpp"
#include "Log.hpp"
#include "SpriteLineParser.hpp"

SuzyProcess::SuzyProcess( Suzy & suzy ) : mSuzy{ suzy }, mScb{ mSuzy.mSCB }, mProcessCoroutine{ process() }, request{}, response{}, mEveron{}
{
}

SuzyProcess::Request const * SuzyProcess::advance()
{
  mProcessCoroutine.resume();
  return &request;
}

void SuzyProcess::respond( uint32_t value )
{
  response.value = value;
}

void SuzyProcess::setFinish()
{
  mSuzy.mSpriteWorking = false;
  request = { Request::FINISH };
}

SuzyProcess::ProcessCoroutine SuzyProcess::process()
{
  auto & scb = mScb;
  auto & suzy = mSuzy;

  while ( ( scb.scbnext & 0xff00 ) != 0 )
  {
    scb.scbadr = scb.scbnext;
    scb.tmpadr = scb.scbadr;

    suzy.writeSPRCTL0( co_await suzyFetchSCB( scb.tmpadr++ ) );
    suzy.writeSPRCTL1( co_await suzyFetchSCB( scb.tmpadr++ ) );
    suzy.writeSPRCOLL( co_await suzyFetchSCB( scb.tmpadr++ ) );
    scb.scbnext.l = co_await suzyFetchSCB( scb.tmpadr++ );
    scb.scbnext.h = co_await suzyFetchSCB( scb.tmpadr++ );

    if ( suzy.mSkipSprite )
      continue;

    scb.sprdline.l = co_await suzyFetchSCB( scb.tmpadr++ );
    scb.sprdline.h = co_await suzyFetchSCB( scb.tmpadr++ );
    scb.hposstrt.l = co_await suzyFetchSCB( scb.tmpadr++ );
    scb.hposstrt.h = co_await suzyFetchSCB( scb.tmpadr++ );
    scb.vposstrt.l = co_await suzyFetchSCB( scb.tmpadr++ );
    scb.vposstrt.h = co_await suzyFetchSCB( scb.tmpadr++ );

    scb.tilt = 0;
    scb.stretch = 0;

    switch ( suzy.mReload )
    {
    case Suzy::Reload::HVST:  //Reload hsize, vsize, stretch, tilt
      scb.sprhsiz.l = co_await suzyFetchSCB( scb.tmpadr++ );
      scb.sprhsiz.h = co_await suzyFetchSCB( scb.tmpadr++ );
      scb.sprvsiz.l = co_await suzyFetchSCB( scb.tmpadr++ );
      scb.sprvsiz.h = co_await suzyFetchSCB( scb.tmpadr++ );
      scb.stretch.l = co_await suzyFetchSCB( scb.tmpadr++ );
      scb.stretch.h = co_await suzyFetchSCB( scb.tmpadr++ );
      scb.tilt.l = co_await suzyFetchSCB( scb.tmpadr++ );
      scb.tilt.h = co_await suzyFetchSCB( scb.tmpadr++ );
      break;
    case Suzy::Reload::HVS:   //Reload hsize, vsize, stretch
      scb.sprhsiz.l = co_await suzyFetchSCB( scb.tmpadr++ );
      scb.sprhsiz.h = co_await suzyFetchSCB( scb.tmpadr++ );
      scb.sprvsiz.l = co_await suzyFetchSCB( scb.tmpadr++ );
      scb.sprvsiz.h = co_await suzyFetchSCB( scb.tmpadr++ );
      scb.stretch.l = co_await suzyFetchSCB( scb.tmpadr++ );
      scb.stretch.h = co_await suzyFetchSCB( scb.tmpadr++ );
      break;
    case Suzy::Reload::HV:    //Reload hsize, vsize
      scb.sprhsiz.l = co_await suzyFetchSCB( scb.tmpadr++ );
      scb.sprhsiz.h = co_await suzyFetchSCB( scb.tmpadr++ );
      scb.sprvsiz.l = co_await suzyFetchSCB( scb.tmpadr++ );
      scb.sprvsiz.h = co_await suzyFetchSCB( scb.tmpadr++ );
      break;
    case Suzy::Reload::NONE:  //Reload nothing
      break;
    }

    if ( !suzy.mReusePalette )
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

      p0 = co_await suzyReadPal( scb.tmpadr );
      scb.tmpadr += 4;
      p1 = co_await suzyReadPal( scb.tmpadr );
      scb.tmpadr += 4;

      //TODO: implement bug:
      //The page break signal does not delay the end of the pen index palette loading.
      for ( size_t i = 0; i < arr.size(); ++i )
      {
        uint8_t value = arr[i];
        suzy.mPalette[2 * i] = (uint8_t)((value >> 4) & 0x0f);
        suzy.mPalette[2 * i + 1] = (uint8_t)(value & 0x0f);
      }
    }

    suzy.mDisableCollisions = suzy.mNoCollide ||
      ( ( suzy.mSprColl & Suzy::SPRCOLL::NO_COLLIDE ) == 1 ) ||
      ( suzy.mSpriteType == Suzy::Sprite::BACKNONCOLL ) ||
      ( suzy.mSpriteType == Suzy::Sprite::NONCOLL );

    suzy.mFred = std::nullopt;

    {

      VidOperator vidOp{ suzy.mSpriteType };
      ColOperator colOp{ suzy.mSpriteType, (uint8_t)(suzy.mSprColl & Suzy::SPRCOLL::NUMBER_MASK) };

      mEveron = false;

      auto const& quadCycle = suzy.mQuadrantOrder[(size_t)suzy.mStartingQuadrant];

      for ( int quadrant = 0; quadrant < 4; ++quadrant )
      {
        int left = ((uint8_t)quadCycle[quadrant] & Suzy::SPRCTL1::DRAW_LEFT) == 0 ? 0 : 1;
        int up = ((uint8_t)quadCycle[quadrant] & Suzy::SPRCTL1::DRAW_UP) == 0 ? 0 : 1;
        left ^= suzy.mHFlip ? 1 : 0;
        const int dx = left ? -1 : 1;
        up ^= suzy.mVFlip ? 1 : 0;
        const int dy = up ? -1 : 1;
        scb.tiltacum = 0;
        scb.vsizacum = up ? 0 : scb.vsizoff.w;
        scb.sprvpos = scb.vposstrt - scb.voff;
        if ( ((uint8_t)quadCycle[quadrant] & Suzy::SPRCTL1::DRAW_UP) != ((uint8_t)quadCycle[(size_t)suzy.mStartingQuadrant] & Suzy::SPRCTL1::DRAW_UP) )
          scb.sprvpos += dy;

        for ( ;; )
        {
          scb.vsizacum += scb.sprvsiz;
          int pixelHeight = scb.vsizacum.h;
          scb.vsizacum.h = 0;
          scb.sprdoff = co_await suzyRead( scb.sprdline );
          if ( scb.sprdoff == 0 )
            break;
          scb.sprdline += 1;
          for ( int pixelRow = 0; pixelRow < pixelHeight; ++pixelRow )
          {
            scb.procadr = scb.sprdline;
            Shifter shifter{};
            shifter.push( co_await suzyRead4( scb.procadr ) );
            scb.procadr += 4;
            SpriteLineParser slp{ shifter, suzy.mLiteral, suzy.bpp(), (scb.sprdoff - 1) * 8 };
            if ( !up && (int16_t)scb.sprvpos >= Suzy::mScreenHeight || up && (int16_t) scb.sprvpos < 0 )
              break;
            if ( (int16_t)scb.sprvpos < Suzy::mScreenHeight && (int16_t)scb.sprvpos >= 0 )
            {
              scb.vidadr = scb.vidbas + scb.sprvpos * Suzy::mScreenWidth / 2;
              scb.colladr = scb.collbas + scb.sprvpos * Suzy::mScreenWidth / 2;
              vidOp.newLine( scb.vidadr );
              colOp.newLine( scb.colladr );
              scb.hposstrt += (int8_t)scb.tiltacum.h;
              scb.tiltacum.h = 0;
              int hsizacum = left ? 0 : scb.hsizoff.w;
              int sprhpos = (int16_t)( scb.hposstrt - scb.hoff );
              if ( ( (uint8_t)quadCycle[quadrant] & Suzy::SPRCTL1::DRAW_LEFT ) != ( (uint8_t)quadCycle[(size_t)suzy.mStartingQuadrant] & Suzy::SPRCTL1::DRAW_LEFT ) )
                sprhpos += dx;

              while ( int const * pen = slp.getPen() )
              {
                if ( shifter.size() < 24 && slp.totalBits() > shifter.size() )
                {
                  shifter.push( co_await suzyRead( scb.procadr ) );
                  scb.procadr += 1;
                }

                hsizacum += scb.sprhsiz;
                uint8_t pixelWidth = hsizacum >> 8;
                hsizacum &= 0xff;

                for ( int pixelCol = 0; pixelCol < pixelWidth; pixelCol++ )
                {
                  // Stop horizontal loop if outside of screen bounds
                  if ( sprhpos >= 0 && sprhpos < Suzy::mScreenWidth )
                  {
                    if ( !suzy.mDisableCollisions )
                    {
                      if ( auto memOp = colOp.process( sprhpos, *pen ) )
                      {
                        colOp.receiveHiColl( co_await suzyColRMW( memOp.mask, memOp.addr, memOp.value ) );
                      }
                    }

                    const uint8_t pixel = suzy.mPalette[*pen];

                    switch ( auto memOp = vidOp.process( sprhpos, pixel ) )
                    {
                    case VidOperator::MemOp::WRITE:
                      co_await suzyWrite( memOp.addr, memOp.value );
                      break;
                    case VidOperator::MemOp::MODIFY:
                    case VidOperator::MemOp::WRITE | VidOperator::MemOp::MODIFY:
                      co_await suzyVidRMW( memOp.addr, memOp.value, memOp.mask() );
                      break;
                    case VidOperator::MemOp::XOR:
                      co_await suzyXOR( memOp.addr, memOp.value );
                      break;
                    default:
                      break;
                    }

                    mEveron = true;
                  }
                  sprhpos += dx;
                }
              }

              switch ( auto memOp = vidOp.flush() )
              {
              case VidOperator::MemOp::XOR:
                co_await suzyXOR( memOp.addr, memOp.value );
                break;
              default:
                co_await suzyVidRMW( memOp.addr, memOp.value, memOp.mask() );
                break;
              }
              if ( !suzy.mDisableCollisions )
              {
                if ( auto memOp = colOp.flush() )
                {
                  colOp.receiveHiColl( co_await suzyColRMW( memOp.mask, memOp.addr, memOp.value ) );
                }
              }
            }
            scb.sprvpos += dy;
            scb.sprhsiz += scb.stretch;
            scb.tiltacum += scb.tilt;
          }
          if ( scb.sprdoff < 2 )
            break;
          scb.sprdline += scb.sprdoff - 1;
          if ( suzy.mVStretch )
            scb.sprvsiz += scb.stretch * pixelHeight;
        }
        if ( scb.sprdoff == 0 )
          break;
      }
      auto fred = colOp.hiColl();
      if ( !suzy.mDisableCollisions && fred )
      {
        suzy.mFred = *fred & 0x0f;
      }
    }

    if ( suzy.mEveron && mEveron )
    {
      suzy.mFred = suzy.mFred.value_or( 0 ) | 0x80;
    }

    if ( suzy.mFred )
    {
      co_await suzyWriteFred( (uint16_t)( scb.scbadr + scb.colloff ), *suzy.mFred );
    }

    if ( suzy.mSpriteStop )
      break;
  }
}

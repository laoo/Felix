#include "pch.hpp"
#include "SuzyProcess.hpp"
#include "VidOperator.hpp"
#include "ColOperator.hpp"
#include "Log.hpp"
#include "SpriteLineParser.hpp"

SuzyProcess::SuzyProcess( Suzy & suzy ) : mSuzy{ suzy }, scb{ mSuzy.mSCB }, mBaseCoroutine{}, mShifter{}, sprhpos{}, hsizacum{}, left{}, mEveron{}
{
  auto p = process();
  mBaseCoroutine = std::move( p );
}

SuzyProcess::Request const * SuzyProcess::advance()
{
  assert( !mCoro.done() );
  mCoro();
  return &request;
}

void SuzyProcess::respond( uint32_t value )
{
  response.value = value;
}

void SuzyProcess::setFinish()
{
  requestFinish = ISuzyProcess::RequestFinish{};
}

void SuzyProcess::setRead( uint16_t address )
{
  requestRead = ISuzyProcess::RequestRead{ address };
}

void SuzyProcess::setRead4( uint16_t address )
{
  requestRead4 = ISuzyProcess::RequestRead4{ address };
}

void SuzyProcess::setWrite( uint16_t address, uint8_t value )
{
  requestWrite = ISuzyProcess::RequestWrite{ address, value };
}

void SuzyProcess::setColRMW( uint16_t address, uint32_t mask, uint8_t value )
{
  requestWrite4 = ISuzyProcess::RequestColRMW{ address, mask, value };
}

void SuzyProcess::setVidRMW( uint16_t address, uint8_t value, uint8_t mask )
{
  requestVidRMW = ISuzyProcess::RequestVidRMW{ address, value, mask };
}

void SuzyProcess::setXor( uint16_t address, uint8_t value )
{
  requestXOR = ISuzyProcess::RequestXOR{ address, value };
}

SuzyProcess::Response const& SuzyProcess::getResponse() const
{
  return response;
}

void SuzyProcess::setHandle( std::coroutine_handle<> c )
{
  mCoro = c;
}

ProcessCoroutine SuzyProcess::process()
{
  co_await this;

  while ( ( scb.scbnext & 0xff00 ) != 0 )
  {
    scb.scbadr = scb.scbnext;
    scb.tmpadr = scb.scbadr;

    mSuzy.writeSPRCTL0( co_await SuzyRead{ scb.tmpadr++ } );
    mSuzy.writeSPRCTL1( co_await SuzyRead{ scb.tmpadr++ } );
    mSuzy.writeSPRCOLL( co_await SuzyRead{ scb.tmpadr++ } );
    scb.scbnext.l = co_await SuzyRead{ scb.tmpadr++ };
    scb.scbnext.h = co_await SuzyRead{ scb.tmpadr++ };

    if ( mSuzy.mSkipSprite )
      co_return;

    scb.sprdline.l = co_await SuzyRead{ scb.tmpadr++ };
    scb.sprdline.h = co_await SuzyRead{ scb.tmpadr++ };
    scb.hposstrt.l = co_await SuzyRead{ scb.tmpadr++ };
    scb.hposstrt.h = co_await SuzyRead{ scb.tmpadr++ };
    scb.vposstrt.l = co_await SuzyRead{ scb.tmpadr++ };
    scb.vposstrt.h = co_await SuzyRead{ scb.tmpadr++ };

    scb.tilt = 0;
    scb.stretch = 0;

    switch ( mSuzy.mReload )
    {
    case Suzy::Reload::HVST:  //Reload hsize, vsize, stretch, tilt
      scb.sprhsiz.l = co_await SuzyRead{ scb.tmpadr++ };
      scb.sprhsiz.h = co_await SuzyRead{ scb.tmpadr++ };
      scb.sprvsiz.l = co_await SuzyRead{ scb.tmpadr++ };
      scb.sprvsiz.h = co_await SuzyRead{ scb.tmpadr++ };
      scb.stretch.l = co_await SuzyRead{ scb.tmpadr++ };
      scb.stretch.h = co_await SuzyRead{ scb.tmpadr++ };
      scb.tilt.l = co_await SuzyRead{ scb.tmpadr++ };
      scb.tilt.h = co_await SuzyRead{ scb.tmpadr++ };
      break;
    case Suzy::Reload::HVS:   //Reload hsize, vsize, stretch
      scb.sprhsiz.l = co_await SuzyRead{ scb.tmpadr++ };
      scb.sprhsiz.h = co_await SuzyRead{ scb.tmpadr++ };
      scb.sprvsiz.l = co_await SuzyRead{ scb.tmpadr++ };
      scb.sprvsiz.h = co_await SuzyRead{ scb.tmpadr++ };
      scb.stretch.l = co_await SuzyRead{ scb.tmpadr++ };
      scb.stretch.h = co_await SuzyRead{ scb.tmpadr++ };
      break;
    case Suzy::Reload::HV:    //Reload hsize, vsize
      scb.sprhsiz.l = co_await SuzyRead{ scb.tmpadr++ };
      scb.sprhsiz.h = co_await SuzyRead{ scb.tmpadr++ };
      scb.sprvsiz.l = co_await SuzyRead{ scb.tmpadr++ };
      scb.sprvsiz.h = co_await SuzyRead{ scb.tmpadr++ };
      break;
    case Suzy::Reload::NONE:  //Reload nothing
      break;
    }

    if ( !mSuzy.mReusePalette )
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

      p0 = co_await SuzyRead4{ scb.tmpadr };
      scb.tmpadr += 4;
      p1 = co_await SuzyRead4{ scb.tmpadr };
      scb.tmpadr += 4;

      //TODO: implement bug:
      //The page break signal does not delay the end of the pen index palette loading.
      for ( size_t i = 0; i < arr.size(); ++i )
      {
        uint8_t value = arr[i];
        mSuzy.mPalette[2 * i] = (uint8_t)((value >> 4) & 0x0f);
        mSuzy.mPalette[2 * i + 1] = (uint8_t)(value & 0x0f);
      }
    }

    mSuzy.mDisableCollisions = mSuzy.mNoCollide |
      ( ( mSuzy.mSprColl & Suzy::SPRCOLL::NO_COLLIDE ) == 1 ) |
      ( mSuzy.mSpriteType == Suzy::Sprite::BACKNONCOLL ) |
      ( mSuzy.mSpriteType == Suzy::Sprite::NONCOLL );

    mSuzy.mFred = std::nullopt;

    co_await renderSingleSprite( this );

    if ( mSuzy.mEveron && mEveron )
    {
      mSuzy.mFred = mSuzy.mFred.value_or( 0 ) | 0x80;
    }

    if ( mSuzy.mFred )
    {
      co_await SuzyWrite{ (uint16_t)( scb.scbadr + scb.colloff ), *mSuzy.mFred };
    }

    if ( mSuzy.mSpriteStop )
      break;
  }

  mSuzy.mSpriteWorking = false;
}

SubCoroutine SuzyProcess::renderSingleSprite( SuzyProcess * s )
{
  co_await s;

  auto & self = *s;

  VidOperator vidOp{ self.mSuzy.mSpriteType };
  ColOperator colOp{ self.mSuzy.mSpriteType, (uint8_t)(self.mSuzy.mSprColl & Suzy::SPRCOLL::NUMBER_MASK) };

  self.mEveron = false;

  auto const& quadCycle = self.mSuzy.mQuadrantOrder[(size_t)self.mSuzy.mStartingQuadrant];

  for ( int quadrant = 0; quadrant < 4; ++quadrant )
  {
    self.left = ((uint8_t)quadCycle[quadrant] & Suzy::SPRCTL1::DRAW_LEFT) == 0 ? 0 : 1;
    int up = ((uint8_t)quadCycle[quadrant] & Suzy::SPRCTL1::DRAW_UP) == 0 ? 0 : 1;
    self.left ^= self.mSuzy.mHFlip ? 1 : 0;
    up ^= self.mSuzy.mVFlip ? 1 : 0;
    self.scb.tiltacum = 0;
    self.scb.vsizacum = (up == 0) ? self.scb.vsizoff.w : 0;
    self.scb.sprvpos = self.scb.vposstrt - self.scb.voff;
    if ( ((uint8_t)quadCycle[quadrant] & Suzy::SPRCTL1::DRAW_UP) != ((uint8_t)quadCycle[(size_t)self.mSuzy.mStartingQuadrant] & Suzy::SPRCTL1::DRAW_UP) )
      self.scb.sprvpos += up ? -1 : 1;

    for ( ;; )
    {
      self.scb.vsizacum.h = 0;
      self.scb.vsizacum += self.scb.sprvsiz;
      uint8_t pixelHeight = self.scb.vsizacum.h;
      for ( int pixelRow = 0; pixelRow < pixelHeight; ++pixelRow, self.scb.sprvpos += up ? -1 : 1 )
      {
        self.scb.procadr = self.scb.sprdline;
        self.mShifter = Shifter{};
        self.mShifter.push( co_await SuzyRead4{ self.scb.procadr } );
        self.scb.procadr += 4;
        self.scb.sprdoff = self.mShifter.pull<8>();
        SpriteLineParser slp{ self.mShifter, self.mSuzy.mLiteral, self.mSuzy.bpp(), (self.scb.sprdoff - 1) * 8 };
        if ( up == 0 && self.scb.sprvpos >= Suzy::mScreenHeight || up == 1 && (int16_t)(self.scb.sprvpos) < 0 ) continue;
        self.scb.vidadr = self.scb.vidbas + self.scb.sprvpos * Suzy::mScreenWidth / 2;
        self.scb.colladr = self.scb.collbas + self.scb.sprvpos * Suzy::mScreenWidth / 2;
        vidOp.newLine( self.scb.vidadr );
        colOp.newLine( self.scb.colladr );
        self.scb.hposstrt += self.scb.tiltacum.h;
        self.scb.tiltacum.h = 0;
        self.hsizacum = self.left == 0 ? self.scb.hsizoff.w : 0;
        self.sprhpos = self.scb.hposstrt - self.scb.hoff;
        if ( ((uint8_t)quadCycle[quadrant] & Suzy::SPRCTL1::DRAW_LEFT) != ((uint8_t)quadCycle[(size_t)self.mSuzy.mStartingQuadrant] & Suzy::SPRCTL1::DRAW_LEFT) )
          self.sprhpos += self.left ? -1 : 1;

        while ( int const* pen = slp.getPen() )
        {
          if ( self.mShifter.size() < 24 && slp.totalBits() > self.mShifter.size() )
          {
            self.mShifter.push( co_await SuzyRead{ self.scb.procadr } );
            self.scb.procadr += 1;
          }

          self.hsizacum += self.scb.sprhsiz;
          uint8_t pixelWidth = self.hsizacum >> 8;
          self.hsizacum &= 0xff;

          for ( int h = 0; h < pixelWidth; h++ )
          {
            // Stop horizontal loop if outside of screen bounds
            if ( self.sprhpos < Suzy::mScreenWidth )
            {
              uint8_t pixel = self.mSuzy.mPalette[*pen];

              if ( !self.mSuzy.mDisableCollisions )
              {
                if ( auto memOp = colOp.process( self.sprhpos, pixel ) )
                {
                  colOp.receiveHiColl( co_await SuzyColRMW{ memOp.mask, memOp.addr, memOp.value } );
                }
              }

              switch ( auto memOp = vidOp.process( self.sprhpos, pixel ) )
              {
              case VidOperator::MemOp::WRITE:
                co_await SuzyWrite{ memOp.addr, memOp.value };
                break;
              case VidOperator::MemOp::MODIFY:
              case VidOperator::MemOp::WRITE | VidOperator::MemOp::MODIFY:
                co_await SuzyVidRMW{ memOp.addr, memOp.value, memOp.mask() };
                break;
              case VidOperator::MemOp::XOR:
                co_await SuzyXOR{ memOp.addr, memOp.value };
                break;
              default:
                break;
              }

              self.mEveron = true;
            }
            self.sprhpos += self.left ? -1 : 1;
          }
        }

        switch ( auto memOp = vidOp.flush() )
        {
        case VidOperator::MemOp::XOR:
          co_await SuzyXOR{ memOp.addr, memOp.value };
          break;
        default:
          co_await SuzyVidRMW{ memOp.addr, memOp.value, memOp.mask() };
          break;
        }
        if ( !self.mSuzy.mDisableCollisions )
        {
          if ( auto memOp = colOp.flush() )
          {
            colOp.receiveHiColl( co_await SuzyColRMW{ memOp.mask, memOp.addr, memOp.value } );
          }
        }
      }
      self.scb.sprdline += self.scb.sprdoff;
      if ( self.scb.sprdoff < 2 )
        break;
    }
    if ( self.scb.sprdoff == 0 )
      break;
  }
  auto fred = colOp.hiColl();
  if ( !self.mSuzy.mDisableCollisions )
    self.mSuzy.mFred = fred & 0x0f;
}

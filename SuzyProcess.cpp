#include "SuzyProcess.hpp"



SuzyProcess::SuzyProcess( Suzy & suzy ) : mSuzy{ suzy }, scb{ mSuzy.mSCB }, mBaseCoroutine{}, mShifter{}, sprhpos{}, hsizacum{}
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

void SuzyProcess::setWrite4( uint16_t address, uint32_t value )
{
  requestWrite4 = ISuzyProcess::RequestWrite4{ address, value };
}

void SuzyProcess::setRMW( uint16_t address, uint8_t value, uint8_t mask )
{
  requestRMW = ISuzyProcess::RequestRMW{ address, value, mask };
}

void SuzyProcess::setXor( uint16_t address, uint8_t value )
{
  requestXOR = ISuzyProcess::RequestXOR{ address, value };
}

SuzyProcess::Response const& SuzyProcess::getResponse() const
{
  return response;
}

void SuzyProcess::setHandle( std::experimental::coroutine_handle<> c )
{
  mCoro = c;
}

AssemblePen & SuzyProcess::assemblePen( int pen, int count )
{
  mAssembledPen.pen = pen;
  mAssembledPen.count = count;

  return mAssembledPen;
}

AssemblePen & SuzyProcess::getPen()
{
  return mAssembledPen;
}

void SuzyProcess::initPen( std::experimental::coroutine_handle<> hCaller, std::experimental::coroutine_handle<> hAssembler )
{
  mAssembledPen.callerHandle = hCaller;
  mAssembledPen.assemblerHandle = hAssembler;
}

ProcessCoroutine SuzyProcess::process()
{
  co_await this;

  while ( ( scb.scbnext & 0xff00 ) != 0 )
  {
    scb.scbadr = scb.scbnext;
    scb.tmpadr = scb.scbadr;

    co_await loadSCB();

    mSuzy.mFred = 0;

    bool isEveronScreen = co_await renderSingleSprite();

    if ( mSuzy.mEveron && !isEveronScreen )
    {
      co_await SuzyRMW{ ( uint16_t )( scb.scbadr + scb.colloff ), 0xff, 0x80 };
    }
  }

  co_return;
}

SubCoroutine SuzyProcess::loadSCB()
{
  co_await this;

  mSuzy.writeSPRCTL0( co_await SuzyRead{ scb.scbadr++ } );
  mSuzy.writeSPRCTL1( co_await SuzyRead{ scb.scbadr++ } );
  mSuzy.mSprColl = co_await SuzyRead{ scb.scbadr++ };
  scb.scbnext.l = co_await SuzyRead{ scb.scbadr++ };
  scb.scbnext.h = co_await SuzyRead{ scb.scbadr++ };

  if ( mSuzy.mSkipSprite )
    co_return;

  scb.sprdline.l = co_await SuzyRead{ scb.scbadr++ };
  scb.sprdline.h = co_await SuzyRead{ scb.scbadr++ };
  scb.hposstrt.l = co_await SuzyRead{ scb.scbadr++ };
  scb.hposstrt.h = co_await SuzyRead{ scb.scbadr++ };
  scb.vposstrt.l = co_await SuzyRead{ scb.scbadr++ };
  scb.vposstrt.h = co_await SuzyRead{ scb.scbadr++ };

  scb.tilt = 0;
  scb.stretch = 0;

  switch ( mSuzy.mReload )
  {
  case Suzy::Reload::HVST:  //Reload hsize, vsize, stretch, tilt
    scb.sprhsiz.l = co_await SuzyRead{ scb.scbadr++ };
    scb.sprhsiz.h = co_await SuzyRead{ scb.scbadr++ };
    scb.sprvsiz.l = co_await SuzyRead{ scb.scbadr++ };
    scb.sprvsiz.h = co_await SuzyRead{ scb.scbadr++ };
    scb.stretch.l = co_await SuzyRead{ scb.scbadr++ };
    scb.stretch.h = co_await SuzyRead{ scb.scbadr++ };
    scb.tilt.l = co_await SuzyRead{ scb.scbadr++ };
    scb.tilt.h = co_await SuzyRead{ scb.scbadr++ };
    break;
  case Suzy::Reload::HVS:   //Reload hsize, vsize, stretch
    scb.sprhsiz.l = co_await SuzyRead{ scb.scbadr++ };
    scb.sprhsiz.h = co_await SuzyRead{ scb.scbadr++ };
    scb.sprvsiz.l = co_await SuzyRead{ scb.scbadr++ };
    scb.sprvsiz.h = co_await SuzyRead{ scb.scbadr++ };
    scb.stretch.l = co_await SuzyRead{ scb.scbadr++ };
    scb.stretch.h = co_await SuzyRead{ scb.scbadr++ };
    break;
  case Suzy::Reload::HV:    //Reload hsize, vsize
    scb.sprhsiz.l = co_await SuzyRead{ scb.scbadr++ };
    scb.sprhsiz.h = co_await SuzyRead{ scb.scbadr++ };
    scb.sprvsiz.l = co_await SuzyRead{ scb.scbadr++ };
    scb.sprvsiz.h = co_await SuzyRead{ scb.scbadr++ };
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

    p0 = co_await SuzyRead4{ scb.scbadr };
    scb.scbadr += 4;
    p1 = co_await SuzyRead4{ scb.scbadr };
    scb.scbadr += 4;

    //TODO: implement bug:
    //The page break signal does not delay the end of the pen index palette loading.
    for ( size_t i = 0; i < arr.size(); ++i )
    {
      uint8_t value = arr[i];
      mSuzy.mPalette[2 * i] = ( uint8_t )( ( value >> 4 ) & 0x0f );
      mSuzy.mPalette[2 * i + 1] = ( uint8_t )( value & 0x0f );
    }
  }
}

SubCoroutineT<bool> SuzyProcess::renderSingleSprite()
{
  co_await this;

  if ( mSuzy.mSkipSprite )
    co_return false;

  auto pa = co_await penAssembler();

  bool isEveron{};

  int bpp = mSuzy.bpp();
  auto const& quadCycle = mSuzy.mQuadrantOrder[( size_t )mSuzy.mStartingQuadrant];

  for ( int quadrant = 0; quadrant < 4; ++quadrant )
  {
    int left = ( ( uint8_t )quadCycle[quadrant] & Suzy::SPRCTL1::DRAW_LEFT ) == 0 ? 0 : 1;
    int up = ( ( uint8_t )quadCycle[quadrant] & Suzy::SPRCTL1::DRAW_UP ) == 0 ? 0 : 1;
    left ^= mSuzy.mHFlip ? 1 : 0;
    up ^= mSuzy.mVFlip ? 1 : 0;
    scb.tiltacum = 0;
    scb.vsizacum = ( up == 0 ) ? scb.vsizoff.w : 0;
    scb.sprvpos = scb.vposstrt - scb.voff;
    if ( ( ( uint8_t )quadCycle[quadrant] & Suzy::SPRCTL1::DRAW_UP ) != ( ( uint8_t )quadCycle[( size_t )mSuzy.mStartingQuadrant] & Suzy::SPRCTL1::DRAW_UP ) )
      scb.sprvpos += up ? -1 : 1;

    for ( ;; )
    {
      scb.procadr = scb.sprdline;
      mShifter.push( co_await SuzyRead4{ scb.procadr } );
      scb.procadr += 4;
      scb.sprdoff = mShifter.pull<8>();
      if ( scb.sprdoff < 2 )
        break;

      int totalBits = ( scb.sprdoff - 1 ) * 8;
      scb.vidadr = scb.vidbas + scb.sprvpos * Suzy::mScreenWidth / 2;
      scb.colladr = scb.collbas + scb.sprvpos * Suzy::mScreenWidth / 2;
      scb.vsizacum += scb.sprvsiz;
      uint8_t pixelHeight = scb.vsizacum.h;
      scb.vsizacum.h = 0;
      for ( int pixelRow = 0; pixelRow < pixelHeight; ++pixelRow )
      {
        if ( up == 0 && scb.sprvpos >= Suzy::mScreenHeight || up == 1 && ( int16_t )( scb.sprvpos ) < 0 ) break;
        scb.hposstrt += scb.tiltacum.h;
        scb.tiltacum.h = 0;
        hsizacum = left == 0 ? scb.hsizoff.w : 0;
        sprhpos = scb.hposstrt - scb.hoff;
        if ( ( ( uint8_t )quadCycle[quadrant] & Suzy::SPRCTL1::DRAW_LEFT ) != ( ( uint8_t )quadCycle[( size_t )mSuzy.mStartingQuadrant] & Suzy::SPRCTL1::DRAW_LEFT ) )
          sprhpos += left ? -1 : 1;

        for ( ;; )  //each pen in line
        {
          if ( mSuzy.mLiteral )
          {
            while ( totalBits > bpp )
            {
              co_await assemblePen( mShifter.pull( bpp ), 1 );
              totalBits -= bpp;
            }
          }
          else
          {
            while ( totalBits > bpp + 1 )
            {
              int literal = mShifter.pull<1>();
              int count = mShifter.pull<4>();

              if ( literal )
              {
                while ( count-- >= 0 && totalBits > bpp )
                {
                  co_await assemblePen( mShifter.pull( bpp ), 1 );
                  sprhpos += left ? -1 : 1;
                  totalBits -= bpp;
                }
              }
              else
              {
                if ( count == 0 )
                {
                  break;
                }
                int pen = mShifter.pull( bpp );
                co_await assemblePen( pen, count );
              }
            }
          }
        }
      }
      scb.sprdline += scb.sprdoff;
      scb.sprvpos += up ? -1 : 1;
    }
  }
  co_return true;
}

PenAssemblerCoroutine SuzyProcess::penAssembler()
{
  co_await this;

  for ( ;; )
  {
    AssemblePen pen = co_await getPen();
    int c = pen.count;
  }
  //hsizacum += scb.sprhsiz;02
  //uint8_t pixelWidth = hsizacum >> 8;
  //hsizacum &= 0xff;

  //// Draw pixel
  //uint8_t pixelValue = mSuzy.mPalette[pixelIndex];

  //for ( int h = 0; h < pixelWidth; h++ )
  //{
  //  // Stop horizontal loop if outside of screen bounds
  //  if ( sprhpos >= 0 && sprhpos < Suzy.SCREEN_WIDTH )
  //  {
  //    // Process pixel based on sprite type
  //    bool left = sprhpos % 2 == 0;
  //    ushort offset = ( ushort )( ( sprhpos + sprvpos * Suzy.SCREEN_WIDTH ) / 2 );
  //    ProcessPixel( ( ushort )( VIDADR.Value + offset ), pixelValue, left );
  //    ProcessCollision( ( ushort )( COLLADR.Value + offset ), pixelValue, left );

  //    isEverOnScreen = true;
  //  }
  //  sprhpos += horizontalIncrease;
  //}

}

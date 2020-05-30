#include "SuzyProcess.hpp"
#include "VidOperator.hpp"
#include "ColOperator.hpp"
#include "Log.hpp"

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

void SuzyProcess::setHandle( std::experimental::coroutine_handle<> c )
{
  mCoro = c;
}

AssemblePen & SuzyProcess::readPen()
{
  mAssembledPen.op = AssemblePen::Op::READ_PEN;
  return mAssembledPen;
}

AssemblePen & SuzyProcess::readHeader()
{
  mAssembledPen.op = AssemblePen::Op::READ_HEADER;
  return mAssembledPen;
}

AssemblePen & SuzyProcess::duplicatePen()
{
  mAssembledPen.op = AssemblePen::Op::DUPLICATE_PEN;
  return mAssembledPen;
}

AssemblePen & SuzyProcess::flush()
{
  mAssembledPen.op = AssemblePen::Op::FLUSH;
  return mAssembledPen;
}

AssemblePen & SuzyProcess::newLine()
{
  mAssembledPen.op = AssemblePen::Op::NEW_LINE;
  return mAssembledPen;
}

AssemblePen & SuzyProcess::finish()
{
  mAssembledPen.op = AssemblePen::Op::FINISH;
  return mAssembledPen;
}

AssemblePen & SuzyProcess::getPen()
{
  return mAssembledPen;
}

void SuzyProcess::initPen( std::experimental::coroutine_handle<> handle )
{
  mAssembledPen.handle = handle;
}

ProcessCoroutine SuzyProcess::process()
{
  co_await this;

  while ( ( scb.scbnext & 0xff00 ) != 0 )
  {
    scb.scbadr = scb.scbnext;
    scb.tmpadr = scb.scbadr;

    co_await loadSCB();

    mSuzy.mDisableCollisions = mSuzy.mNoCollide |
      ( ( mSuzy.mSprColl & Suzy::SPRCOLL::NO_COLLIDE ) == 1 ) |
      ( mSuzy.mSpriteType == Suzy::Sprite::BACKNONCOLL ) |
      ( mSuzy.mSpriteType == Suzy::Sprite::NONCOLL );

    mSuzy.mFred = std::nullopt;

    co_await renderSingleSprite();

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

SubCoroutine SuzyProcess::loadSCB()
{
  co_await this;

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
      mSuzy.mPalette[2 * i] = ( uint8_t )( ( value >> 4 ) & 0x0f );
      mSuzy.mPalette[2 * i + 1] = ( uint8_t )( value & 0x0f );
    }
  }
}

SubCoroutine SuzyProcess::renderSingleSprite()
{
  co_await this;

  if ( mSuzy.mSkipSprite )
    co_return;

  auto pa = co_await penAssembler();

  mEveron = false;

  int bpp = mSuzy.bpp();
  auto const& quadCycle = mSuzy.mQuadrantOrder[( size_t )mSuzy.mStartingQuadrant];

  for ( int quadrant = 0; quadrant < 4; ++quadrant )
  {
    left = ( ( uint8_t )quadCycle[quadrant] & Suzy::SPRCTL1::DRAW_LEFT ) == 0 ? 0 : 1;
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
      mShifter = Shifter{};
      mShifter.push( co_await SuzyRead4{ scb.procadr } );
      scb.procadr += 4;
      scb.sprdoff = mShifter.pull<8>();
      if ( scb.sprdoff < 2 )
        break;

      int totalBits = ( scb.sprdoff - 1 ) * 8;
      scb.vsizacum += scb.sprvsiz;
      uint8_t pixelHeight = scb.vsizacum.h;
      scb.vsizacum.h = 0;
      for ( int pixelRow = 0; pixelRow < pixelHeight; ++pixelRow )
      {
        if ( up == 0 && scb.sprvpos >= Suzy::mScreenHeight || up == 1 && ( int16_t )( scb.sprvpos ) < 0 ) break;
        scb.vidadr = scb.vidbas + scb.sprvpos * Suzy::mScreenWidth / 2;
        scb.colladr = scb.collbas + scb.sprvpos * Suzy::mScreenWidth / 2;
        co_await newLine();
        scb.hposstrt += scb.tiltacum.h;
        scb.tiltacum.h = 0;
        hsizacum = left == 0 ? scb.hsizoff.w : 0;
        sprhpos = scb.hposstrt - scb.hoff;
        if ( ( ( uint8_t )quadCycle[quadrant] & Suzy::SPRCTL1::DRAW_LEFT ) != ( ( uint8_t )quadCycle[( size_t )mSuzy.mStartingQuadrant] & Suzy::SPRCTL1::DRAW_LEFT ) )
          sprhpos += left ? -1 : 1;

        if ( mSuzy.mLiteral )
        {
          while ( totalBits > bpp )
          {
            co_await readPen();
            totalBits -= bpp;
          }
        }
        else
        {
          while ( totalBits > bpp + 1 )
          {
            auto [count, literal] = co_await readHeader();

            if ( literal )
            {
              while ( count-- >= 0 && totalBits > bpp )
              {
                co_await readPen();
                totalBits -= bpp;
              }
            }
            else
            {
              if ( count == 0 )
              {
                break;
              }
              co_await readPen();
              while ( --count >= 0 )
                co_await duplicatePen();
            }
          }
        }
        co_await flush();
        scb.sprvpos += up ? -1 : 1;
      }
      scb.sprdline += scb.sprdoff;
    }
    if ( scb.sprdoff == 0 )
      break;
  }
  auto [fred, _] = co_await finish();
  if ( !mSuzy.mDisableCollisions )
    mSuzy.mFred = fred & 0x0f;
}

PenAssemblerCoroutine SuzyProcess::penAssembler()
{
  co_await this;

  VidOperator vidOp{ mSuzy.mSpriteType };
  ColOperator colOp{ mSuzy.mSpriteType, (uint8_t)( mSuzy.mSprColl & Suzy::SPRCOLL::NUMBER_MASK ) };

  int bpp = mSuzy.bpp();

  std::array<int, (size_t)AssemblePen::Op::SIZE_> bitsToRead = {
    bpp,
    5
  };

  int pen{};

  for ( ;; )
  {
    auto & ap = co_await getPen();

    int bits = bitsToRead[( int )ap.op];
    if ( mShifter.size() < bits )
    {
      mShifter.push( co_await SuzyRead4{ scb.procadr } );
      scb.procadr += 4;
    }

    switch ( ap.op )
    {
    case AssemblePen::Op::READ_PEN:
      pen = mShifter.pull( bpp );
      break;
    case AssemblePen::Op::READ_HEADER:
      ap.literal = mShifter.pull<1>();
      ap.count = mShifter.pull<4>();
      continue;
    case AssemblePen::Op::NEW_LINE:
      vidOp.newLine( scb.vidadr );
      colOp.newLine( scb.colladr );
      continue;
    case AssemblePen::Op::FLUSH:
      switch ( auto memOp = vidOp.flush() )
      {
      case VidOperator::MemOp::XOR:
        co_await SuzyXOR{ memOp.addr, memOp.value };
        break;
      default:
        co_await SuzyVidRMW{ memOp.addr, memOp.value, memOp.mask() };
        break;
      }
      if ( !mSuzy.mDisableCollisions )
      {
        if( auto memOp = colOp.flush() )
        {
          colOp.receiveHiColl( co_await SuzyColRMW{ memOp.mask, memOp.addr, memOp.value } );
        }
      }
      continue;
    case AssemblePen::Op::FINISH:
      ap.count = colOp.hiColl();
      continue;
    case AssemblePen::Op::DUPLICATE_PEN:
      break;
    default:
      assert( false );
      continue;
    }

    hsizacum += scb.sprhsiz;
    uint8_t pixelWidth = hsizacum >> 8;
    hsizacum &= 0xff;

    for ( int h = 0; h < pixelWidth; h++ )
    {
      // Stop horizontal loop if outside of screen bounds
      if ( sprhpos >= 0 && sprhpos < Suzy::mScreenWidth )
      {
        uint8_t pixel = mSuzy.mPalette[pen];

        if ( !mSuzy.mDisableCollisions )
        {
          if ( auto memOp = colOp.process( sprhpos, pixel ) )
          {
            colOp.receiveHiColl( co_await SuzyColRMW{ memOp.mask, memOp.addr, memOp.value } );
          }
        }

        switch ( auto memOp = vidOp.process( sprhpos, pixel ) )
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

        mEveron = true;
      }
      sprhpos += left ? -1 : 1;
    }
  }
}

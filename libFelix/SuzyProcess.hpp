#pragma once
#include "Suzy.hpp"
#include "Utility.hpp"
#include "SuzyProcess.hpp"
#include "VidOperator.hpp"
#include "ColOperator.hpp"
#include "Log.hpp"
#include "SpriteLineParser.hpp"

struct DummyDumper
{
  void startSprite( uint16_t windowAddress, int16_t posx, int16_t posy, bool background )
  {
  }

  uint8_t fetch( uint8_t value )
  {
    return value;
  }

  uint32_t fetch( uint32_t value )
  {
    return value;
  }

  void drawByte( uint16_t address, uint8_t value, uint8_t mask )
  {
  }
};

struct SuzyProcessResponse
{
  bool await_ready() { return false; }
  void await_suspend( std::coroutine_handle<> c ) {}
  uint32_t value;
};

template< typename SPRITEDUMPER>
class SuzyProcess : public ISuzyProcess
{

public:

  SuzyProcess( Suzy & suzy, SPRITEDUMPER& sink ) : mSuzy{ suzy }, mProcessCoroutine{ process() }, request{}, response{}, mSink{ sink }
  {
  }

  ~SuzyProcess() override = default;

  Request const* advance() override
  {
    if ( mSuzy.mSpriteWorking )
      mProcessCoroutine.resume();
    else
      setFinish();

    return &request;
  }

  void respond( uint32_t value ) override
  {
    response.value = value;
  }

private:

  void setFinish()
  {
    mSuzy.mSpriteWorking = false;
    request = { Request::FINISH };
  }

  //reads one byte of sprite data
  auto & suzyRead( uint16_t address )
  {
    struct SuzyReadResponse : public SuzyProcessResponse
    {
      uint8_t await_resume() { return (uint8_t)value; }
    };
    request = { Request::READ, address };
    return static_cast<SuzyReadResponse &>( response );
  }

  //reads four bytes of sprite data
  auto& suzyRead4( uint16_t address )
  {
    struct SuzyRead4Response : public SuzyProcessResponse
    {
      uint32_t await_resume() { return value; }
    };
    request = { Request::READ4, address };
    return static_cast< SuzyRead4Response& >( response );
  }

  //reads SCB data
  auto & suzyFetchSCB( uint16_t address )
  {
    struct SuzyFetchSCBResponse : public SuzyProcessResponse
    {
      uint8_t await_resume() { return (uint8_t)value; }
    };
    request = { Request::FETCHSCB, address };
    return static_cast<SuzyFetchSCBResponse &>( response );
  }

  //reads pen indices data
  auto & suzyReadPal( uint16_t address )
  {
    struct SuzyReadPalResponse : public SuzyProcessResponse
    {
      uint32_t await_resume() { return value; }
    };
    request = { Request::READPAL, address };
    return static_cast<SuzyReadPalResponse &>( response );
  }

  //performs color data write
  auto & suzyWrite( uint16_t address, uint8_t value )
  {
    mSink.drawByte( address, value, 0 );
    struct SuzyWriteResponse : public SuzyProcessResponse
    {
      void await_resume() {}
    };
    request = { Request::WRITE, address, value };
    return static_cast<SuzyWriteResponse &>( response );
  }

  //FRED write-back 
  auto & suzyWriteFred( uint16_t address, uint8_t value )
  {
    struct SuzyWriteResponse : public SuzyProcessResponse
    {
      void await_resume() {}
    };
    request = { Request::WRITEFRED, address, value };
    return static_cast<SuzyWriteResponse &>( response );
  }

  //performs collision data RMW
  auto & suzyColRMW( uint32_t mask, uint16_t address, uint16_t value )
  {
    struct SuzyColRMWResponse : public SuzyProcessResponse
    {
      uint32_t await_resume() { return value; }
    };
    request = { Request::COLRMW, address, value, mask };
    return static_cast<SuzyColRMWResponse &>( response );
  }

  //performs color data RMW
  auto & suzyVidRMW( uint16_t address, uint8_t value, uint8_t mask )
  {
    mSink.drawByte( address, value, mask );
    struct SuzyVidRMWResponse : public SuzyProcessResponse
    {
      void await_resume() {}
    };
    request = { Request::VIDRMW, address, value, mask };
    return static_cast<SuzyVidRMWResponse &>( response );
  }

  //performs XOR RMW
  auto & suzyXOR( uint16_t address, uint8_t value )
  {
    struct SuzyXORResponse : public SuzyProcessResponse
    {
      void await_resume() {}
    };
    request = { Request::XOR, address, value };
    return static_cast<SuzyXORResponse &>( response );
  }

  struct ProcessCoroutine : private NonCopyable
  {
  public:
    struct promise_type;
    using handle = std::coroutine_handle<promise_type>;

    struct promise_type
    {
      promise_type( SuzyProcess & suzyProcess ) : mSuzyProcess{ suzyProcess } {}
      auto get_return_object() { return ProcessCoroutine{ std::coroutine_handle<promise_type>::from_promise( *this ) }; }
      std::suspend_always initial_suspend() { return {}; }
      void return_void() {}
      void unhandled_exception() { std::terminate(); }

      std::suspend_always final_suspend() noexcept
      {
        mSuzyProcess.setFinish();
        return {};
      }

    private:
      SuzyProcess & mSuzyProcess;
    };

    ProcessCoroutine( handle c ) : mCoro{ c } {}
    ~ProcessCoroutine()
    {
      if ( mCoro )
        mCoro.destroy();
    }

    void resume() const
    {
      assert( !mCoro.done() );
      mCoro();
    }

  private:
    handle mCoro;
  } const mProcessCoroutine;

private:
  ProcessCoroutine process()
  {
    auto& suzy = mSuzy;
    auto& scb = mSuzy.mSCB;

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
        uint32_t p0 = co_await suzyReadPal( scb.tmpadr );
        scb.tmpadr += 4;

        //TODO: implement bug:
        //The page break signal does not delay the end of the pen index palette loading.
        suzy.mPalette[0x0] = ( p0 >> ( 0 * 8 + 4 ) ) & 0x0f;
        suzy.mPalette[0x1] = ( p0 >> ( 0 * 8 + 0 ) ) & 0x0f;
        suzy.mPalette[0x2] = ( p0 >> ( 1 * 8 + 4 ) ) & 0x0f;
        suzy.mPalette[0x3] = ( p0 >> ( 1 * 8 + 0 ) ) & 0x0f;
        suzy.mPalette[0x4] = ( p0 >> ( 2 * 8 + 4 ) ) & 0x0f;
        suzy.mPalette[0x5] = ( p0 >> ( 2 * 8 + 0 ) ) & 0x0f;
        suzy.mPalette[0x6] = ( p0 >> ( 3 * 8 + 4 ) ) & 0x0f;
        suzy.mPalette[0x7] = ( p0 >> ( 3 * 8 + 0 ) ) & 0x0f;

        uint32_t p1 = co_await suzyReadPal( scb.tmpadr );
        scb.tmpadr += 4;

        suzy.mPalette[0x8] = ( p1 >> ( 0 * 8 + 4 ) ) & 0x0f;
        suzy.mPalette[0x9] = ( p1 >> ( 0 * 8 + 0 ) ) & 0x0f;
        suzy.mPalette[0xa] = ( p1 >> ( 1 * 8 + 4 ) ) & 0x0f;
        suzy.mPalette[0xb] = ( p1 >> ( 1 * 8 + 0 ) ) & 0x0f;
        suzy.mPalette[0xc] = ( p1 >> ( 2 * 8 + 4 ) ) & 0x0f;
        suzy.mPalette[0xd] = ( p1 >> ( 2 * 8 + 0 ) ) & 0x0f;
        suzy.mPalette[0xe] = ( p1 >> ( 3 * 8 + 4 ) ) & 0x0f;
        suzy.mPalette[0xf] = ( p1 >> ( 3 * 8 + 0 ) ) & 0x0f;
      }

      bool disableCollisions = suzy.mNoCollide ||
        ( ( suzy.mSprColl & Suzy::SPRCOLL::NO_COLLIDE ) == Suzy::SPRCOLL::NO_COLLIDE ) ||
        ( suzy.mSpriteType == Suzy::Sprite::BACKNONCOLL ) ||
        ( suzy.mSpriteType == Suzy::Sprite::NONCOLL );
      bool everon = false;
      std::optional<uint8_t> fred = std::nullopt;

      {

        mSink.startSprite( scb.vidbas, scb.hposstrt.w - scb.hoff, scb.vposstrt - scb.voff, suzy.mSpriteType == Suzy::Sprite::BACKNONCOLL || suzy.mSpriteType == Suzy::Sprite::BACKGROUND );

        VidOperator vidOp{ suzy.mSpriteType };
        ColOperator colOp{ suzy.mSpriteType, ( uint8_t )( suzy.mSprColl & Suzy::SPRCOLL::NUMBER_MASK ) };

        auto const& quadCycle = suzy.mQuadrantOrder[( size_t )suzy.mStartingQuadrant];

        for ( int quadrant = 0; quadrant < 4; ++quadrant )
        {
          int left = ( ( uint8_t )quadCycle[quadrant] & Suzy::SPRCTL1::DRAW_LEFT ) == 0 ? 0 : 1;
          int up = ( ( uint8_t )quadCycle[quadrant] & Suzy::SPRCTL1::DRAW_UP ) == 0 ? 0 : 1;
          left ^= suzy.mHFlip ? 1 : 0;
          const int dx = left ? -1 : 1;
          up ^= suzy.mVFlip ? 1 : 0;
          const int dy = up ? -1 : 1;
          scb.tiltacum = 0;
          scb.vsizacum = up ? 0 : scb.vsizoff.w;
          scb.sprvpos = scb.vposstrt - scb.voff;
          // Comment in Handy:
          // Take the sign of the first quad (0) as the basic sign, all other quads drawing in the other direction get offset by 1 pixel in the other direction, this fixes the squashed look on the multi-quad sprites.
          if ( ( ( uint8_t )quadCycle[quadrant] & Suzy::SPRCTL1::DRAW_UP ) != ( ( uint8_t )quadCycle[0] & Suzy::SPRCTL1::DRAW_UP ) )
            scb.sprvpos += dy;

          for ( ;; )
          {
            scb.vsizacum += scb.sprvsiz;
            int pixelHeight = scb.vsizacum.h;
            scb.vsizacum.h = 0;
            scb.sprdoff = mSink.fetch( co_await suzyRead( scb.sprdline ) );
            if ( scb.sprdoff == 0 )
              break;
            scb.sprdline += 1;
            for ( int pixelRow = 0; pixelRow < pixelHeight; ++pixelRow )
            {
              scb.procadr = scb.sprdline;
              Shifter shifter{};
              shifter.push( mSink.fetch( co_await suzyRead4( scb.procadr ) ) );
              scb.procadr += 4;
              SpriteLineParser slp{ shifter, suzy.mLiteral, suzy.bpp(), ( scb.sprdoff - 1 ) * 8 };
              if ( !up && ( int16_t )scb.sprvpos >= SCREEN_HEIGHT || up && ( int16_t )scb.sprvpos < 0 )
                break;
              if ( ( int16_t )scb.sprvpos < SCREEN_HEIGHT && ( int16_t )scb.sprvpos >= 0 )
              {
                scb.vidadr = scb.vidbas + scb.sprvpos * ROW_BYTES;
                scb.colladr = scb.collbas + scb.sprvpos * ROW_BYTES;
                vidOp.newLine( scb.vidadr );
                colOp.newLine( scb.colladr );
                scb.hposstrt += ( int8_t )scb.tiltacum.h;
                scb.tiltacum.h = 0;
                int hsizacum = left ? 0 : scb.hsizoff.w;
                int sprhpos = ( int16_t )( scb.hposstrt - scb.hoff );
                // Comment in Handy:
                // Take the sign of the first quad (0) as the basic sign, all other quads drawing in the other direction get offset by 1 pixel in the other direction, this fixes the squashed look on the multi-quad sprites.
                if ( ( ( uint8_t )quadCycle[quadrant] & Suzy::SPRCTL1::DRAW_LEFT ) != ( ( uint8_t )quadCycle[0] & Suzy::SPRCTL1::DRAW_LEFT ) )
                  sprhpos += dx;

                while ( int const* penIndex = slp.getPenIndex() )
                {
                  if ( shifter.size() < 24 && slp.totalBits() > shifter.size() )
                  {
                    shifter.push( mSink.fetch( co_await suzyRead( scb.procadr ) ) );
                    scb.procadr += 1;
                  }

                  hsizacum += scb.sprhsiz;
                  uint8_t pixelWidth = hsizacum >> 8;
                  hsizacum &= 0xff;

                  for ( int pixelCol = 0; pixelCol < pixelWidth; pixelCol++ )
                  {
                    // Stop horizontal loop if outside of screen bounds
                    if ( sprhpos >= 0 && sprhpos < SCREEN_WIDTH )
                    {
                      const uint8_t penNumber = suzy.mPalette[*penIndex];

                      if ( !disableCollisions )
                      {
                        if ( auto memOp = colOp.process( sprhpos, penNumber ) )
                        {
                          colOp.receiveHiColl( co_await suzyColRMW( memOp.mask, memOp.addr, memOp.value ) );
                        }
                      }

                      switch ( auto memOp = vidOp.process( sprhpos, penNumber ) )
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

                      everon = true;
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
                if ( !disableCollisions )
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

        if ( !disableCollisions )
        {
          if ( auto newFred = colOp.hiColl() )
          {
            fred = *newFred & 0x0f;
          }
        }
      }

      if ( suzy.mEveron && everon )
      {
        fred = fred.value_or( 0 ) | 0x80;
      }

      if ( fred )
      {
        co_await suzyWriteFred( ( uint16_t )( scb.scbadr + scb.colloff ), *fred );
      }

      if ( suzy.mSpriteStop )
        break;
    }
  }


private:
  Suzy & mSuzy;

  Request request;
  SuzyProcessResponse response;
  SPRITEDUMPER & mSink;
};

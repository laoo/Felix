#include "SuzyProcess.hpp"


namespace
{
template<bool suspend>
struct Init
{
  auto await_transform( SuzyProcess * suzyProcess )
  {
    struct Awaiter
    {
      SuzyProcess * suzyProcess;
      bool await_ready() { return !suspend; }
      void await_resume() {}
      void await_suspend( std::experimental::coroutine_handle<> c ) {  suzyProcess->setHandle( c ); }
    };
    mSuzyProcess = suzyProcess;
    return Awaiter{ suzyProcess };
  }
  SuzyProcess * suzyProcess()
  {
    return mSuzyProcess;
  }

private:
  SuzyProcess * mSuzyProcess;
};

template<typename T>
struct Final
{
  std::experimental::suspend_always final_suspend() noexcept
  {
    SuzyProcess * p = static_cast< T* >( this )->suzyProcess();
    p->setFinish();
    return {};
  }
};

template<typename T>
struct Return
{
  auto final_suspend() noexcept
  {
    auto caller = static_cast< T* >( this )->caller();
    struct Awaiter
    {
      std::experimental::coroutine_handle<> caller;
      bool await_ready() { return false; }
      void await_resume() {}
      auto await_suspend( std::experimental::coroutine_handle<> c ) { return caller; }
    };
    return Awaiter{ caller };
  }
};

template<typename T>
struct Requests
{
  auto await_transform( SuzyRead r )
  {
    struct Awaiter
    {
      SuzyProcess * p;
      bool await_ready() { return false; }
      uint8_t await_resume() { return p->getResponse().value; }
      void await_suspend( std::experimental::coroutine_handle<> c ) { p->setHandle( c ); }
    };
    SuzyProcess * p = static_cast< T* >( this )->suzyProcess();
    p->setRead( r.address );
    return Awaiter{ p };
  }
  auto await_transform( SuzyRead4 r )
  {
    struct Awaiter
    {
      SuzyProcess * p;
      bool await_ready() { return false; }
      uint32_t await_resume() { return p->getResponse().value; }
      void await_suspend( std::experimental::coroutine_handle<> c ) { p->setHandle( c ); }
    };
    SuzyProcess * p = static_cast< T* >( this )->suzyProcess();
    p->setRead4( r.address );
    return Awaiter{ p };
  }
  auto await_transform( SuzyWrite w )
  {
    struct Awaiter
    {
      SuzyProcess * p;
      bool await_ready() { return false; }
      void await_resume() {}
      void await_suspend( std::experimental::coroutine_handle<> c ) { p->setHandle( c ); }
    };
    SuzyProcess * p = static_cast< T* >( this )->suzyProcess();
    p->setWrite( w.address, w.value );
    return Awaiter{ p };
  }
  auto await_transform( SuzyWrite4 w )
  {
    struct Awaiter
    {
      SuzyProcess * p;
      bool await_ready() { return false; }
      void await_resume() {}
      void await_suspend( std::experimental::coroutine_handle<> c ) { p->setHandle( c ); }
    };
    SuzyProcess * p = static_cast< T* >( this )->suzyProcess();
    p->setWrite4( w.address, w.value );
    return Awaiter{ p };
  }
  auto await_transform( SuzyRMW rmw )
  {
    struct Awaiter
    {
      SuzyProcess * p;
      bool await_ready() { return false; }
      void await_resume() {}
      void await_suspend( std::experimental::coroutine_handle<> c ) { p->setHandle( c ); }
    };
    SuzyProcess * p = static_cast< T* >( this )->suzyProcess();
    p->setRMW( rmw.address, rmw.value, rmw.mask );
    return Awaiter{ p };
  }
  auto await_transform( SuzyXOR x )
  {
    struct Awaiter
    {
      SuzyProcess * p;
      bool await_ready() { return false; }
      void await_resume() {}
      void await_suspend( std::experimental::coroutine_handle<> c ) { p->setHandle( c ); }
    };
    SuzyProcess * p = static_cast< T* >( this )->suzyProcess();
    p->setXor( x.address, x.value );
    return Awaiter{ p };
  }
};

}


namespace std::experimental
{
template<>
struct coroutine_traits<SuzyProcess::SubCoroutine, SuzyProcess*>
{
  struct promise_type :
    public BasePromise<SuzyProcess::SubCoroutine>,
    public promise::initial::always,
    public Return<promise_type>,
    public promise::ret_void,
    public Init<false>,
    public Requests<promise_type>
  {
    using Init<false>::await_transform;
    using Requests<promise_type>::await_transform;
  };
};

template<typename RET>
struct coroutine_traits<SuzyProcess::SubCoroutineT<RET>, SuzyProcess*>
{
  struct promise_type :
    public BasePromise<SuzyProcess::SubCoroutineT<RET>>,
    public promise::initial::never,
    public Return<promise_type>,
    public promise::ret_value<RET>,
    public Init<false>,
    public Requests<promise_type>
  {
    using Init<false>::await_transform;
    using Requests<promise_type>::await_transform;
  };
};

}

struct CallSubCoroutine
{
  auto await_transform( SuzyProcess::SubCoroutine && sub )
  {
    struct Awaiter
    {
      SuzyProcess::SubCoroutine sub;
      Awaiter( SuzyProcess::SubCoroutine && s ) : sub{ std::move( s ) } {}
      bool await_ready() { return false; }
      void await_resume() {}
      auto await_suspend( std::experimental::coroutine_handle<> c )
      {
        //auto callee = std::experimental::coroutine_handle<std::experimental::coroutine_traits<SuzyProcess::SubCoroutine, SuzyProcess*>::promise_type>::from_address(
        //  sub.coro().address()
        //);
        //callee.promise().setCaller( c );
        return sub.coro();
      }
    };
    return Awaiter{ std::move( sub ) };
  }

  template<typename RET>
  auto await_transform( SuzyProcess::SubCoroutineT<RET> && sub )
  {
    struct Awaiter
    {
      SuzyProcess::SubCoroutineT<RET> sub;
      Awaiter( SuzyProcess::SubCoroutineT<RET> && s ) : sub{ std::move( s ) } {}
      bool await_ready() { return false; }
      void await_resume() {}
      auto await_suspend( std::experimental::coroutine_handle<> c )
      {
        auto callee = std::experimental::coroutine_handle<std::experimental::coroutine_traits<SuzyProcess::SubCoroutineT<RET>, SuzyProcess*>::promise_type>::from_address(
          sub.coro().address()
        );
        callee.promise().setCaller( c );
        return callee;
      }
    };
    return Awaiter{ std::move( sub ) };
  }
};

namespace std::experimental
{
template<>
struct coroutine_traits<SuzyProcess::ProcessCoroutine, SuzyProcess*>
{
  struct promise_type :
    public BasePromise<SuzyProcess::ProcessCoroutine>,
    public promise::initial::never,
    public promise::ret_void,
    public Init<true>,
    public Requests<promise_type>,
    public CallSubCoroutine,
    public Final<promise_type>
  {
    using Init<true>::await_transform;
    using Requests<promise_type>::await_transform;
    using CallSubCoroutine::await_transform;
  };
};
}



SuzyProcess::SuzyProcess( Suzy & suzy ) : mSuzy{ suzy }, scb{ mSuzy.mSCB }, mBaseCoroutine{}
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



SuzyProcess::ProcessCoroutine SuzyProcess::process()
{
  co_await this;

  while ( ( scb.scbnext & 0xff00 ) != 0 )
  {
    scb.scbadr = scb.scbnext;
    scb.tmpadr = scb.scbadr;

    auto l = loadSCB();

    l();

    mSuzy.mFred = 0;

    bool isEveronScreen = true; // co_await renderSingleSprite();

    if ( mSuzy.mEveron && !isEveronScreen )
    {
      co_await SuzyRMW{ ( uint16_t )( scb.scbadr + scb.colloff ), 0xff, 0x80 };
    }
  }

  co_return;
}

SuzyProcess::SubCoroutine SuzyProcess::loadSCB()
{
  co_await this;
  co_await SuzyRead{ scb.scbadr++ };
  co_return;


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

SuzyProcess::SubCoroutineT<bool> SuzyProcess::renderSingleSprite()
{
  co_await this;

  co_return true;
}

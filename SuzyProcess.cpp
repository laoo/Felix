#include "SuzyProcess.hpp"


namespace
{
struct Init
{
  auto await_transform( SuzyProcess * suzyProcess )
  {
    struct Awaiter
    {
      SuzyProcess * suzyProcess;
      bool await_ready() { return false; }
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
struct coroutine_traits<BaseCoroutine, SuzyProcess*>
{
  struct promise_type : BasePromise
  <
    BaseCoroutine,
    promise::initial::never,
    promise::ret_void,
    Init,
    Requests<promise_type>,
    Final<promise_type>
  >
  {
    using Init::await_transform;
    using Requests<promise_type>::await_transform;
  };
};
}


SuzyProcess::SuzyProcess( Suzy & suzy ) : mSuzy{ suzy }, mBaseCoroutine{ process() }
{
}

SuzyProcess::Request const * SuzyProcess::advance()
{
  mBaseCoroutine();
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

BaseCoroutine SuzyProcess::process()
{
  co_await this;
  uint8_t v = co_await SuzyRead{ 0x200 };
  co_await SuzyWrite{ 0, v };
  co_return;
}

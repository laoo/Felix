#pragma once

struct SuzyRead { uint16_t address; };
struct SuzyRead4
{
  SuzyRead4( uint16_t adr ) : address{ adr }
  {
    if ( adr == 0xfffd )
    {
      int k = 42;
    }
  }
  uint16_t address;
};
struct SuzyWrite { uint16_t address; uint8_t value; };
struct SuzyColRMW { uint32_t mask; uint16_t address;  uint8_t value; };
struct SuzyVidRMW { uint16_t address; uint8_t value; uint8_t mask; };
struct SuzyXOR { uint16_t address; uint8_t value; };


//promise components
namespace coro
{
namespace promise
{

template<typename Coro, typename Promise>
struct Base
{
  void unhandled_exception() { std::terminate(); }

  void setCaller( std::coroutine_handle<> c )
  {
    mCaller = c;
  }

  std::coroutine_handle<> caller() noexcept
  {
    return mCaller;
  }

  auto get_return_object() { return Coro{ std::coroutine_handle<Promise>::from_promise( *(Promise*)this ) }; }


private:
  std::coroutine_handle<> mCaller;
};

}

template<typename Promise>
class Base
{
public:
  using promise_type = Promise;
  using handle = std::coroutine_handle<promise_type>;

  Base() : mCoro{} {}
  Base( handle c ) : mCoro{ c } {}
  Base( Base const& other ) = delete;
  Base & operator=( Base const& other ) = delete;
  Base( Base && other ) noexcept : mCoro{ std::move( other.mCoro ) }
  {
    other.mCoro = nullptr;
  }
  Base & operator=( Base && other ) noexcept
  {
    mCoro = std::move( other.mCoro );
    other.mCoro = nullptr;
    return *this;
  }
  ~Base()
  {
    if ( mCoro )
      mCoro.destroy();
  }

  void resume()
  {
    assert( !mCoro.done() );
    mCoro();
  }

  handle coro()
  {
    return mCoro;
  }

private:
  handle mCoro;
};

}

template<typename Coroutine>
struct CoroutinePromise :
  public coro::promise::Base<Coroutine, CoroutinePromise<Coroutine>>
{
  auto initial_suspend() { return std::suspend_never{}; }
  std::suspend_always final_suspend() noexcept
  {
    SuzyProcess * p = static_cast<CoroutinePromise<Coroutine>*>(this)->suzyProcess();
    p->setFinish();
    return {};
  }
  void return_void()
  {
  }

  auto await_transform( SuzyRead r )
  {
    struct Awaiter
    {
      SuzyProcess * p;
      bool await_ready() { return false; }
      uint8_t await_resume() { return p->getResponse().value; }
      void await_suspend( std::coroutine_handle<> c ) { p->setHandle( c ); }
    };
    SuzyProcess * p = static_cast<CoroutinePromise<Coroutine>*>(this)->suzyProcess();
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
      void await_suspend( std::coroutine_handle<> c ) { p->setHandle( c ); }
    };
    SuzyProcess * p = static_cast<CoroutinePromise<Coroutine>*>(this)->suzyProcess();
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
      void await_suspend( std::coroutine_handle<> c ) { p->setHandle( c ); }
    };
    SuzyProcess * p = static_cast<CoroutinePromise<Coroutine>*>(this)->suzyProcess();
    p->setWrite( w.address, w.value );
    return Awaiter{ p };
  }
  auto await_transform( SuzyColRMW w )
  {
    struct Awaiter
    {
      SuzyProcess * p;
      bool await_ready() { return false; }
      uint8_t await_resume() { return (uint8_t)p->getResponse().value; }
      void await_suspend( std::coroutine_handle<> c ) { p->setHandle( c ); }
    };
    SuzyProcess * p = static_cast<CoroutinePromise<Coroutine>*>(this)->suzyProcess();
    p->setColRMW( w.address, w.mask, w.value );
    return Awaiter{ p };
  }
  auto await_transform( SuzyVidRMW rmw )
  {
    struct Awaiter
    {
      SuzyProcess * p;
      bool await_ready() { return false; }
      void await_resume() {}
      void await_suspend( std::coroutine_handle<> c ) { p->setHandle( c ); }
    };
    SuzyProcess * p = static_cast<CoroutinePromise<Coroutine>*>(this)->suzyProcess();
    p->setVidRMW( rmw.address, rmw.value, rmw.mask );
    return Awaiter{ p };
  }
  auto await_transform( SuzyXOR x )
  {
    struct Awaiter
    {
      SuzyProcess * p;
      bool await_ready() { return false; }
      void await_resume() {}
      void await_suspend( std::coroutine_handle<> c ) { p->setHandle( c ); }
    };
    SuzyProcess * p = static_cast<CoroutinePromise<Coroutine>*>(this)->suzyProcess();
    p->setXor( x.address, x.value );
    return Awaiter{ p };
  }
  auto await_transform( SuzyProcess * suzyProcess )
  {
    struct Awaiter
    {
      SuzyProcess * suzyProcess;
      bool await_ready() { return false; }
      void await_resume() {}
      void await_suspend( std::coroutine_handle<> c ) { suzyProcess->setHandle( c ); }
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

struct ProcessCoroutine : public coro::Base<CoroutinePromise<ProcessCoroutine>>
{
};

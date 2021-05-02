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
namespace initial
{
struct always
{
  auto initial_suspend() { return std::suspend_always{}; }
};
struct never
{
  auto initial_suspend() { return std::suspend_never{}; }
};
}
struct ret_void
{
  void return_void()
  {
  }
};
template<typename T>
struct ret_value
{
  T retValue;
  void return_value( T value )
  {
    retValue = value;
  }

  T getRetValue() const
  {
    return retValue;
  }
};

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

template<typename T>
struct Final
{
  std::suspend_always final_suspend() noexcept
  {
    SuzyProcess * p = static_cast<T*>( this )->suzyProcess();
    p->setFinish();
    return {};
  }
};

template<typename T>
struct Return
{
  auto final_suspend() noexcept
  {
    auto caller = static_cast<T*>( this )->caller();
    struct Awaiter
    {
      std::coroutine_handle<> caller;
      bool await_ready() noexcept { return false; }
      void await_resume() noexcept {}
      auto await_suspend( std::coroutine_handle<> c ) noexcept { return caller; }
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
      void await_suspend( std::coroutine_handle<> c ) { p->setHandle( c ); }
    };
    SuzyProcess * p = static_cast<T*>( this )->suzyProcess();
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
    SuzyProcess * p = static_cast<T*>( this )->suzyProcess();
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
    SuzyProcess * p = static_cast<T*>( this )->suzyProcess();
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
    SuzyProcess * p = static_cast<T*>( this )->suzyProcess();
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
    SuzyProcess * p = static_cast<T*>( this )->suzyProcess();
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
    SuzyProcess * p = static_cast<T*>( this )->suzyProcess();
    p->setXor( x.address, x.value );
    return Awaiter{ p };
  }
};

struct CallSubCoroutine
{
  template<typename SUB>
  auto await_transform( SUB && sub )
  {
    struct Awaiter
    {
      SUB sub;
      Awaiter( SUB && s ) : sub{ std::move( s ) } {}
      bool await_ready() { return false; }
      void await_resume() {}
      auto await_suspend( std::coroutine_handle<> c )
      {
        sub.coro().promise().setCaller( c );
        return sub.coro();
      }
    };
    return Awaiter{ std::move( sub ) };
  }

  template<typename RET, template<typename> typename SUB>
  auto await_transform( SUB<RET> && sub )
  {
    struct Awaiter
    {
      SUB<RET> sub;
      Awaiter( SUB<RET> && s ) : sub{ std::move( s ) } {}
      bool await_ready() { return false; }
      RET await_resume()
      {
        return sub.coro().promise().getRetValue();
      }
      auto await_suspend( std::coroutine_handle<> c )
      {
        sub.coro().promise().setCaller( c );
        return sub.coro();
      }
    };
    return Awaiter{ std::move( sub ) };
  }
};

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
  public coro::promise::Base<Coroutine, CoroutinePromise<Coroutine>>,
  public coro::promise::initial::never,
  public coro::promise::ret_void,
  public coro::promise::Init<true>,
  public coro::promise::Requests<CoroutinePromise<Coroutine>>,
  public coro::promise::CallSubCoroutine,
  public coro::promise::Final<CoroutinePromise<Coroutine>>
{
  using coro::promise::Init<true>::await_transform;
  using coro::promise::Requests<CoroutinePromise<Coroutine>>::await_transform;
  using coro::promise::CallSubCoroutine::await_transform;
};


template<typename Coroutine>
struct SubCoroutinePromise :
  public coro::promise::Base<Coroutine, SubCoroutinePromise<Coroutine>>,
  public coro::promise::initial::always,
  public coro::promise::Return<SubCoroutinePromise<Coroutine>>,
  public coro::promise::ret_void,
  public coro::promise::Init<false>,
  public coro::promise::Requests<SubCoroutinePromise<Coroutine>>
{
  using coro::promise::Init<false>::await_transform;
  using coro::promise::Requests<SubCoroutinePromise<Coroutine>>::await_transform;

};

template<typename Coroutine, typename RET>
struct SubCoroutinePromiseT :
  public coro::promise::Base<Coroutine, SubCoroutinePromiseT<Coroutine, RET>>,
  public coro::promise::initial::always,
  public coro::promise::Return<SubCoroutinePromiseT<Coroutine, RET>>,
  public coro::promise::ret_value<RET>,
  public coro::promise::Init<false>,
  public coro::promise::Requests<SubCoroutinePromiseT<Coroutine, RET>>
{
  using coro::promise::Init<false>::await_transform;
  using coro::promise::Requests<SubCoroutinePromiseT<Coroutine, RET>>::await_transform;
};


struct SubCoroutine : public coro::Base<SubCoroutinePromise<SubCoroutine>>
{
};

template<typename RET>
struct SubCoroutineT : public coro::Base<SubCoroutinePromiseT<SubCoroutineT<RET>, RET>>
{
};


struct ProcessCoroutine : public coro::Base<CoroutinePromise<ProcessCoroutine>>
{
};

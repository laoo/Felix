#pragma once
#include <experimental/coroutine>
#include <cassert>
#include <type_traits>

//struct SuzyRead { uint16_t address; };
//struct SuzyRead4 { uint16_t address; };
//struct SuzyWrite { uint16_t address; uint8_t value; };
//struct SuzyWrite4 { uint16_t address; uint32_t value; };
//struct SuzyRMW { uint16_t address; uint8_t value; uint8_t mask; };
//struct SuzyXOR { uint16_t address; uint8_t value; };


//promise components
namespace promise
{
namespace initial
{
struct always
{
  auto initial_suspend() { return std::experimental::suspend_always{}; }
};
struct never
{
  auto initial_suspend() { return std::experimental::suspend_never{}; }
};
}
namespace final
{
struct always
{
  auto final_suspend() { return std::experimental::suspend_always{}; }
};
struct never
{
  auto final_suspend() { return std::experimental::suspend_never{}; }
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
      void await_suspend( std::experimental::coroutine_handle<> c ) { suzyProcess->setHandle( c ); }
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
      void await_suspend( std::experimental::coroutine_handle<> c ) { p->setHandle( c ); }
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
      void await_suspend( std::experimental::coroutine_handle<> c ) { p->setHandle( c ); }
    };
    SuzyProcess * p = static_cast<T*>( this )->suzyProcess();
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
    SuzyProcess * p = static_cast<T*>( this )->suzyProcess();
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
    SuzyProcess * p = static_cast<T*>( this )->suzyProcess();
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
      auto await_suspend( std::experimental::coroutine_handle<> c )
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
      auto await_suspend( std::experimental::coroutine_handle<> c )
      {
        sub.coro().promise().setCaller( c );
        return sub.coro();
      }
    };
    return Awaiter{ std::move( sub ) };
  }
};

}


template<typename Coro>
struct BasePromise
{
  void unhandled_exception() { std::terminate(); }

  void setCaller( std::experimental::coroutine_handle<> c )
  {
    mCaller = c;
  }

  std::experimental::coroutine_handle<> caller() noexcept
  {
    return mCaller;
  }

private:
  std::experimental::coroutine_handle<> mCaller;
};

template<typename ProcessCoroutine>
struct SubCoroutinePromise :
  public BasePromise<ProcessCoroutine>,
  public promise::initial::always,
  public promise::Return<SubCoroutinePromise<ProcessCoroutine>>,
  public promise::ret_void,
  public promise::Init<false>,
  public promise::Requests<SubCoroutinePromise<ProcessCoroutine>>
{
public:

  using promise::Init<false>::await_transform;
  using promise::Requests<SubCoroutinePromise<ProcessCoroutine>>::await_transform;

  auto get_return_object() { return ProcessCoroutine{ std::experimental::coroutine_handle<SubCoroutinePromise<ProcessCoroutine>>::from_promise( *this ) }; }
};

template<typename ProcessCoroutine, typename RET>
struct SubCoroutinePromiseT :
  public BasePromise<ProcessCoroutine>,
  public promise::initial::always,
  public promise::Return<SubCoroutinePromiseT<ProcessCoroutine, RET>>,
  public promise::ret_value<RET>,
  public promise::Init<false>,
  public promise::Requests<SubCoroutinePromiseT<ProcessCoroutine, RET>>
{
public:

  using promise::Init<false>::await_transform;
  using promise::Requests<SubCoroutinePromiseT<ProcessCoroutine, RET>>::await_transform;

  auto get_return_object() { return ProcessCoroutine{ std::experimental::coroutine_handle<SubCoroutinePromiseT<ProcessCoroutine, RET>>::from_promise( *this ) }; }
};


template<typename ProcessCoroutine>
struct CoroutinePromise :
  public BasePromise<ProcessCoroutine>,
  public promise::initial::never,
  public promise::ret_void,
  public promise::Init<true>,
  public promise::Requests<CoroutinePromise<ProcessCoroutine>>,
  public promise::CallSubCoroutine,
  public promise::Final<CoroutinePromise<ProcessCoroutine>>
{
public:
  using promise::Init<true>::await_transform;
  using promise::Requests<CoroutinePromise<ProcessCoroutine>>::await_transform;
  using promise::CallSubCoroutine::await_transform;

  auto get_return_object() { return ProcessCoroutine{ std::experimental::coroutine_handle<CoroutinePromise<ProcessCoroutine>>::from_promise( *this ) }; }
};


template<typename PROMISE>
class BaseCoroutine
{
public:
  using promise_type = PROMISE;
  using handle = std::experimental::coroutine_handle<promise_type>;

  BaseCoroutine() : mCoro{} {}
  BaseCoroutine( handle c ) : mCoro{ c } {}
  BaseCoroutine( BaseCoroutine const& other ) = delete;
  BaseCoroutine & operator=( BaseCoroutine const& other ) = delete;
  BaseCoroutine( BaseCoroutine && other ) noexcept : mCoro{ std::move( other.mCoro ) }
  {
    other.mCoro = nullptr;
  }
  BaseCoroutine & operator=( BaseCoroutine && other ) noexcept
  {
    mCoro = std::move( other.mCoro );
    other.mCoro = nullptr;
    return *this;
  }
  ~BaseCoroutine()
  {
    if ( mCoro )
      mCoro.destroy();
  }

  void operator()()
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


struct SubCoroutine : public BaseCoroutine<SubCoroutinePromise<SubCoroutine>>
{
};

template<typename RET>
struct SubCoroutineT : public BaseCoroutine<SubCoroutinePromiseT<SubCoroutineT<RET>, RET>>
{
};


struct ProcessCoroutine : public BaseCoroutine<CoroutinePromise<ProcessCoroutine>>
{
};

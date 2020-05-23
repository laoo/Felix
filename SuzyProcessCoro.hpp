#pragma once
#include <experimental/coroutine>
#include <cassert>
#include <type_traits>
#include <optional>

//struct SuzyRead { uint16_t address; };
//struct SuzyRead4 { uint16_t address; };
//struct SuzyWrite { uint16_t address; uint8_t value; };
//struct SuzyWrite4 { uint16_t address; uint32_t value; };
//struct SuzyRMW { uint16_t address; uint8_t value; uint8_t mask; };
//struct SuzyXOR { uint16_t address; uint8_t value; };


//promise components
namespace coro
{
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

template<typename Coro, typename Promise>
struct Base
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

  auto get_return_object() { return Coro{ std::experimental::coroutine_handle<Promise>::from_promise( *(Promise*)this ) }; }


private:
  std::experimental::coroutine_handle<> mCaller;
};

}

template<typename Promise>
class Base
{
public:
  using promise_type = Promise;
  using handle = std::experimental::coroutine_handle<promise_type>;

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

struct AssemblePen
{ 
  enum class Op
  {
    READ_PEN,
    READ_HEADER,
    DUPLICATE_PEN,
    FLUSH
  } op;
  int count;
  bool literal;
  std::experimental::coroutine_handle<> handle;
};


template<typename Coroutine>
struct PenAssemblerPromise :
  public coro::promise::Base<Coroutine, PenAssemblerPromise<Coroutine>>,
  public coro::promise::initial::always,
  public coro::promise::Return<PenAssemblerPromise<Coroutine>>,
  public coro::promise::ret_void,
  public coro::promise::Init<false>,
public coro::promise::Requests<PenAssemblerPromise<Coroutine>>
{
  using coro::promise::Init<false>::await_transform;
  using coro::promise::Requests<PenAssemblerPromise<Coroutine>>::await_transform;
  using coro::promise::Base<Coroutine, PenAssemblerPromise<Coroutine>>::caller;
  using coro::promise::Base<Coroutine, PenAssemblerPromise<Coroutine>>::setCaller;

  auto await_transform( AssemblePen & pen )
  {
    struct Awaiter
    {
      AssemblePen & pen;
      bool await_ready() { return false; }
      AssemblePen & await_resume(){ return pen; }
      auto await_suspend( std::experimental::coroutine_handle<> c )
      {
        std::swap( c, pen.handle );
        return c;
      }
    };
    return Awaiter{ pen };
  }

};

struct PenAssemblerCoroutine : public coro::Base<PenAssemblerPromise<PenAssemblerCoroutine>>
{
};

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

  auto await_transform( PenAssemblerCoroutine && pac )
  {
    struct Awaiter
    {
      PenAssemblerCoroutine && pac;
      SuzyProcess * p;
      bool await_ready() { return false; }
      PenAssemblerCoroutine await_resume() { return std::move( pac ); }
      auto await_suspend( std::experimental::coroutine_handle<> c )
      {
        p->initPen( c );
        return pac.coro();
      }
    };
    return Awaiter{ std::move( pac ), suzyProcess() };
  }

  auto await_transform( AssemblePen & pen )
  {
    struct Awaiter
    {
      AssemblePen & pen;
      bool await_ready() { return false; }
      std::pair<int, bool> await_resume() { return std::make_pair( pen.count, pen.literal ); }
      auto await_suspend( std::experimental::coroutine_handle<> c )
      {
        std::swap( c, pen.handle );
        return c;
      }
    };
    return Awaiter{ pen };
  }

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

#pragma once
#include <experimental/coroutine>
#include <cassert>
#include <type_traits>

struct SuzyRequest
{
  enum class Op : uint8_t
  {
    NONE = 0,
    READ,
    READ4,
    WRITE,
    WRITE4,
    MASKED_RMW,
    XOR,
    _SIZE
  };

  union
  {
    uint64_t raw;
    struct
    {
      uint32_t value;
      uint16_t address;
      Op operation;
      uint8_t mask;
    };
  };

  SuzyRequest() : raw{} {}

  void operator()()
  {
    coro();
  }

  explicit operator bool() const
  {
    return raw != 0;
  }

  std::experimental::coroutine_handle<> coro;
};

struct SuzyRead { uint16_t address; };
struct SuzyRead4 { uint16_t address; };
struct SuzyRMW { uint16_t address; uint8_t mask; uint8_t value; };

class SuzyCoSubroutine
{
public:

  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;

  struct promise_type
  {
    auto get_return_object() { return SuzyCoSubroutine{ handle::from_promise( *this ) }; }
    auto initial_suspend() { return std::experimental::suspend_always{}; }
    auto final_suspend() noexcept
    {
      struct Awaiter
      {
        std::experimental::coroutine_handle<> outer;
        bool await_ready() { return false; }
        void await_resume() {}
        auto await_suspend( std::experimental::coroutine_handle<> c ) { return outer; }
      };
      return Awaiter{ outer };
    }

    void return_void()
    {
    }

    void unhandled_exception() { std::terminate(); }
    auto await_transform( SuzyRequest & req )
    {
      mReq = &req;
      return std::experimental::suspend_never{};
    }
    auto await_transform( SuzyRead read )
    {
      struct Awaiter
      {
        SuzyRequest * req;
        bool await_ready() { return false; }
        uint8_t await_resume() { return ( uint8_t )req->value; }
        void await_suspend( std::experimental::coroutine_handle<> c ) { req->coro = c; }
      };
      mReq->operation = SuzyRequest::Op::READ;
      mReq->address = read.address;
      return Awaiter{ mReq };
    }
    auto await_transform( SuzyRead4 read )
    {
      struct Awaiter
      {
        SuzyRequest * req;
        bool await_ready() { return false; }
        uint32_t await_resume() { return req->value; }
        void await_suspend( std::experimental::coroutine_handle<> c ) { req->coro = c; }
      };
      mReq->operation = SuzyRequest::Op::READ4;
      mReq->address = read.address;
      return Awaiter{ mReq };
    }
    auto await_transform( SuzyRMW rmw )
    {
      struct Awaiter
      {
        SuzyRequest * req;
        bool await_ready() { return false; }
        void await_resume() {}
        void await_suspend( std::experimental::coroutine_handle<> c ) { req->coro = c; }
      };
      mReq->operation = SuzyRequest::Op::MASKED_RMW;
      mReq->address = rmw.address;
      mReq->value = rmw.value;
      mReq->mask = rmw.mask;
      return Awaiter{ mReq };
    }
    SuzyRequest * mReq;
    std::experimental::coroutine_handle<> outer;
  };

  SuzyCoSubroutine( handle c ) : coro{ c } {}
  SuzyCoSubroutine( SuzyCoSubroutine const & other ) = delete;
  SuzyCoSubroutine& operator=( SuzyCoSubroutine const & other ) = delete;
  SuzyCoSubroutine( SuzyCoSubroutine && other ) noexcept : coro{ std::move( other.coro ) }
  {
    other.coro = nullptr;
  }
  SuzyCoSubroutine & operator=( SuzyCoSubroutine && other ) noexcept
  {
    std::swap( coro, other.coro );
    return *this;
  }
  ~SuzyCoSubroutine()
  {
    if ( coro )
      coro.destroy();
  }

  void setOuter( std::experimental::coroutine_handle<> outer )
  {
    coro.promise().outer = outer;
  }

  std::experimental::coroutine_handle<> getInner() const
  {
    return coro;
  }

private:
  handle coro;
};


template<typename T>
class SuzyCoSubroutineT
{
public:

  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;

  struct promise_type
  {
    auto get_return_object() { return SuzyCoSubroutineT{ handle::from_promise( *this ) }; }
    auto initial_suspend() { return std::experimental::suspend_always{}; }
    auto final_suspend() noexcept
    {
      struct Awaiter
      {
        std::experimental::coroutine_handle<> outer;
        bool await_ready() { return false; }
        void await_resume() {}
        auto await_suspend( std::experimental::coroutine_handle<> c ) { return outer; }
      };
      return Awaiter{ outer };
    }

    void return_value( T value )
    {
      mReturnValue = value;
    }

    void unhandled_exception() { std::terminate(); }
    auto await_transform( SuzyRequest & req )
    {
      mReq = &req;
      return std::experimental::suspend_never{};
    }
    auto await_transform( SuzyRead read )
    {
      struct Awaiter
      {
        SuzyRequest * req;
        bool await_ready() { return false; }
        uint8_t await_resume() { return ( uint8_t )req->value; }
        void await_suspend( std::experimental::coroutine_handle<> c ) { req->coro = c; }
      };
      mReq->operation = SuzyRequest::Op::READ;
      mReq->address = read.address;
      return Awaiter{ mReq };
    }
    auto await_transform( SuzyRead4 read )
    {
      struct Awaiter
      {
        SuzyRequest * req;
        bool await_ready() { return false; }
        uint32_t await_resume() { return req->value; }
        void await_suspend( std::experimental::coroutine_handle<> c ) { req->coro = c; }
      };
      mReq->operation = SuzyRequest::Op::READ4;
      mReq->address = read.address;
      return Awaiter{ mReq };
    }
    auto await_transform( SuzyRMW rmw )
    {
      struct Awaiter
      {
        SuzyRequest * req;
        bool await_ready() { return false; }
        void await_resume() {}
        void await_suspend( std::experimental::coroutine_handle<> c ) { req->coro = c; }
      };
      mReq->operation = SuzyRequest::Op::MASKED_RMW;
      mReq->address = rmw.address;
      mReq->value = rmw.value;
      mReq->mask = rmw.mask;
      return Awaiter{ mReq };
    }
    SuzyRequest * mReq;
    std::experimental::coroutine_handle<> outer;
    T mReturnValue;
  };

  SuzyCoSubroutineT( handle c ) : coro{ c } {}
  SuzyCoSubroutineT( SuzyCoSubroutineT const & other ) = delete;
  SuzyCoSubroutineT& operator=( SuzyCoSubroutineT const & other ) = delete;
  SuzyCoSubroutineT( SuzyCoSubroutineT && other ) noexcept : coro{ std::move( other.coro ) }
  {
    other.coro = nullptr;
  }
  SuzyCoSubroutineT & operator=( SuzyCoSubroutineT && other ) noexcept
  {
    std::swap( coro, other.coro );
    return *this;
  }
  ~SuzyCoSubroutineT()
  {
    if ( coro )
      coro.destroy();
  }

  void setOuter( std::experimental::coroutine_handle<> outer )
  {
    coro.promise().outer = outer;
  }

  std::experimental::coroutine_handle<> getInner() const
  {
    return coro;
  }

  T getResult() const
  {
    return coro.promise().mReturnValue;
  }

private:
  handle coro;
};



class SuzySpriteProcessor
{
public:

  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;

  struct promise_type
  {
    auto get_return_object() { return SuzySpriteProcessor{ handle::from_promise( *this ) }; }
    auto initial_suspend() { return std::experimental::suspend_never{}; }
    auto final_suspend() noexcept { return std::experimental::suspend_always{}; }
    void return_void()
    {
      mReq->raw = 0;
    }
    void unhandled_exception() { std::terminate(); }
    auto await_transform( SuzyRequest & req )
    {
      mReq = &req;
      struct Awaiter
      {
        SuzyRequest * req;
        bool await_ready() { return false; }
        void await_resume() {}
        void await_suspend( std::experimental::coroutine_handle<> c ) { req->coro = c; }
      };
      return Awaiter{ &req };
    }
    auto await_transform( SuzyCoSubroutine && suzyCoro )
    {
      struct Awaiter
      {
        SuzyCoSubroutine coro;
        Awaiter( SuzyCoSubroutine && s ) : coro{ std::move( s ) } {}
        bool await_ready() { return false; }
        void await_resume() {}
        auto await_suspend( std::experimental::coroutine_handle<> c ) { coro.setOuter( c ); return coro.getInner(); }
      };
      return Awaiter{ std::move( suzyCoro ) };
    }
    template<typename T>
    auto await_transform( SuzyCoSubroutineT<T> && suzyCoro )
    {
      struct Awaiter
      {
        SuzyCoSubroutineT<T> coro;
        Awaiter( SuzyCoSubroutineT<T> && s ) : coro{ std::move( s ) } {}
        bool await_ready() { return false; }
        T await_resume()
        {
          return coro.getResult();
        }
        auto await_suspend( std::experimental::coroutine_handle<> c ) { coro.setOuter( c ); return coro.getInner(); }
      };
      return Awaiter{ std::move( suzyCoro ) };
    }
    auto await_transform( SuzyRMW rmw )
    {
      struct Awaiter
      {
        SuzyRequest * req;
        bool await_ready() { return false; }
        void await_resume() {}
        void await_suspend( std::experimental::coroutine_handle<> c ) { req->coro = c; }
      };
      mReq->operation = SuzyRequest::Op::MASKED_RMW;
      mReq->address = rmw.address;
      mReq->value = rmw.value;
      mReq->mask = rmw.mask;
      return Awaiter{ mReq };
    }

    SuzyRequest * mReq;
  };

  SuzySpriteProcessor() : coro{} {}
  SuzySpriteProcessor( handle c ) : coro{ c } {}
  SuzySpriteProcessor( SuzySpriteProcessor const & other ) = delete;
  SuzySpriteProcessor & operator=( SuzySpriteProcessor const & other ) = delete;
  SuzySpriteProcessor( SuzySpriteProcessor && other ) noexcept : coro{ std::move( other.coro ) }
  {
    other.coro = nullptr;
  }
  SuzySpriteProcessor & operator=( SuzySpriteProcessor && other ) noexcept
  {
    std::swap( coro, other.coro );
    return *this;
  }
  ~SuzySpriteProcessor()
  {
    if ( coro )
      coro.destroy();
  }

private:
  handle coro;

};



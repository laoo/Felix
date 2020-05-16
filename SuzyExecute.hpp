#pragma once
#include <experimental/coroutine>

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
    raw = 0;
    coro();
  }

  explicit operator bool() const
  {
    return raw != 0;
  }

  std::experimental::coroutine_handle<> coro;
};


struct AwaitSuzyFetchSCB
{
  SuzyRequest * req;

  bool await_ready()
  {
    return false;
  }

  uint8_t await_resume()
  {
    return req->value;
  }

  void await_suspend( std::experimental::coroutine_handle<> c )
  {
    req->coro = c;
  }
};

struct AwaitSuzyFetchSprite
{
  SuzyRequest * req;

  bool await_ready()
  {
    return false;
  }

  uint8_t await_resume()
  {
    return req->value;
  }

  void await_suspend( std::experimental::coroutine_handle<> c )
  {
    req->coro = c;
  }
};

struct AwaitSuzyReadPixel
{
  SuzyRequest * req;

  bool await_ready()
  {
    return false;
  }

  uint8_t await_resume()
  {
    return req->value;
  }

  void await_suspend( std::experimental::coroutine_handle<> c )
  {
    req->coro = c;
  }
};

struct AwaitSuzyRequest
{
  SuzyRequest * req;

  bool await_ready()
  {
    return false;
  }

  void await_resume()
  {
  }

  void await_suspend( std::experimental::coroutine_handle<> c )
  {
    req->coro = c;
  }
};

struct SuzyExecute
{
  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;

  struct promise_type
  {
    auto get_return_object() { return SuzyExecute{ handle::from_promise( *this ) }; }
    auto initial_suspend() { return std::experimental::suspend_never{}; }
    auto final_suspend() noexcept { return std::experimental::suspend_always{}; }
    void return_void();
    void unhandled_exception() { std::terminate(); }
    AwaitSuzyRequest await_transform( SuzyRequest & req );

    SuzyRequest * mReq;
  };


  SuzyExecute() : coro{}
  {
  }

  SuzyExecute( handle c ) : coro{ c }
  {
  }

  SuzyExecute( SuzyExecute const & other ) = delete;
  SuzyExecute & operator=( SuzyExecute const & other ) = delete;
  SuzyExecute & operator=( SuzyExecute && other ) noexcept
  {
    std::swap( coro, other.coro );
    return *this;
  }


  ~SuzyExecute()
  {
    if ( coro )
      coro.destroy();
  }

  handle coro;
};


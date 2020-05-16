#pragma once
#include <experimental/coroutine>
#include "Suzy.hpp"

class BusMaster;

struct SuzyFetchSCB
{
  uint16_t address;

  struct Tag {};

  SuzyFetchSCB( uint16_t a, Tag t ) : address{ a } {}
};

struct SuzyFetchSprite
{
  uint16_t address;

  struct Tag {};

  SuzyFetchSprite( uint16_t a, Tag t ) : address{ a } {}
};

struct SuzyReadPixel
{
  uint16_t address;

  SuzyReadPixel( uint16_t a ) : address{ a } {}
};

struct SuzyWritePixel
{
  uint16_t address;
  uint8_t value;

  SuzyWritePixel( uint16_t a, uint8_t v ) : address{ a }, value{ v } {}
};

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

  bool operator()()
  {
    raw = 0;
    coro();
    return raw != 0;
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

struct AwaitSuzyWritePixel
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
    auto initial_suspend() { return std::experimental::suspend_always{}; }
    auto final_suspend() noexcept { return std::experimental::suspend_always{}; }
    void return_void();
    void unhandled_exception() { std::terminate(); }
    AwaitSuzyFetchSCB yield_value( SuzyFetchSCB f );
    AwaitSuzyFetchSprite yield_value( SuzyFetchSprite f );
    AwaitSuzyReadPixel yield_value( SuzyReadPixel r );
    AwaitSuzyWritePixel yield_value( SuzyWritePixel w );

    BusMaster * mBus;
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

SuzyExecute suzyExecute( Suzy & suzy, BusMaster & bus );

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
  enum class Type
  {
    NONE,
    FETCH_SCB,
    FETCH_SPRITE,
    READ_PIXEL,
    WRITE_PIXEL,
  } mType;

  uint16_t address;
  uint8_t value;

  SuzyRequest() : mType{ Type::NONE }, address{}, value{} {}
  SuzyRequest( SuzyFetchSCB r ) : mType{ Type::FETCH_SCB }, address{ r.address }, value{} {}
  SuzyRequest( SuzyFetchSprite r ) : mType{ Type::FETCH_SPRITE }, address{ r.address }, value{} {}
  SuzyRequest( SuzyReadPixel r ) : mType{ Type::READ_PIXEL }, address{ r.address }, value{} {}
  SuzyRequest( SuzyWritePixel w ) : mType{ Type::WRITE_PIXEL }, address{ w.address }, value{ w.value } {}

  void resume()
  {
    mType = Type::NONE;
    coro.resume();
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
    auto get_return_object() { return SuzyExecute{ this, handle::from_promise( *this ) }; }
    auto initial_suspend() { return std::experimental::suspend_always{}; }
    auto final_suspend() noexcept { return std::experimental::suspend_always{}; }
    void return_void() {}
    void unhandled_exception() { std::terminate(); }
    AwaitSuzyFetchSCB yield_value( SuzyFetchSCB f );
    AwaitSuzyFetchSprite yield_value( SuzyFetchSprite f );
    AwaitSuzyReadPixel yield_value( SuzyReadPixel r );
    AwaitSuzyWritePixel yield_value( SuzyWritePixel w );

    BusMaster * mBus;
  };

  SuzyExecute() : promise{}, coro{}
  {
  }

  SuzyExecute( promise_type * promise, handle c ) : promise{ promise }, coro{ c }
  {
  }

  ~SuzyExecute()
  {
    if ( coro )
      coro.destroy();
  }

  void setBusMaster( BusMaster * bus )
  {
    promise->mBus = bus;
    coro.resume();
  }

  handle coro;
  promise_type * promise;
};

SuzyExecute suzyExecute( Suzy & suzy );

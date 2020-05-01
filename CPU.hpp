#pragma once
#include <cstdint>
#include <experimental/coroutine>

class BusMaster;

class CPU
{
  uint8_t mA;
  uint8_t mX;
  uint8_t mY;
  uint16_t mS;
  uint16_t mPC;
  int mInt;

  static const int I_NONE  = 0;
  static const int I_IRQ = 1;
  static const int I_NMI = 2;
  static const int I_RESET = 4;
};

struct Read
{
  uint16_t address;

  Read( uint16_t a ) : address{ a } {}
};

struct Write
{
  uint16_t address;
  uint8_t value;

  Write( uint16_t a, uint8_t v ) : address{ a }, value{ v } {}
};


struct CPURequest
{
  enum class Type
  {
    NONE,
    READ,
    WRITE,
  } mType;

  uint16_t address;
  uint8_t value;

  CPURequest() : mType{ Type::NONE }, address{}, value{} {}
  CPURequest( Read r ) : mType{ Type::READ }, address{ r.address }, value{} {}
  CPURequest( Write w ) : mType{ Type::WRITE }, address{ w.address }, value{ w.value } {}

  void resume()
  {
    mType = Type::NONE;
    coro.resume();
  }

  std::experimental::coroutine_handle<> coro;
};

struct AwaitRead
{
  CPURequest * req;

  bool await_ready()
  {
    return false;
  }

  int await_resume()
  {
    return req->value;
  }

  void await_suspend( std::experimental::coroutine_handle<> c )
  {
    req->coro = c;
  }
};

struct AwaitWrite
{
  CPURequest * req;

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

struct CpuLoop
{
  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;

  struct promise_type
  {
    auto get_return_object() { return CpuLoop{ *this, handle::from_promise( *this ) }; }
    auto initial_suspend() { return std::experimental::suspend_always{}; }
    auto final_suspend() noexcept { return std::experimental::suspend_always{}; }
    void return_void() {}
    void unhandled_exception() { std::terminate(); }
    AwaitRead yield_value( Read r );
    AwaitWrite yield_value( Write r );

    BusMaster * mBus;
  };

  CpuLoop( promise_type & promise, handle c ) : promise{ promise }, coro{ c }
  {
  }

  ~CpuLoop()
  {
    if ( coro )
      coro.destroy();
  }

  void setBusMaster( BusMaster * bus )
  {
    promise.mBus = bus;
    coro.resume();
  }

  handle coro;
  promise_type & promise;
};

CpuLoop cpuLoop( CPU & cpu );

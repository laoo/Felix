#pragma once
#include <experimental/coroutine>
#include "CPU.hpp"

class BusMaster;

struct OpInt
{
  Opcode op;
  int interrupt;
};

struct CPUReadOpcode
{
  uint16_t address;

  CPUReadOpcode( uint16_t a, Opcode o ) : address{ a } {}
};

struct CPURead
{
  uint16_t address;

  CPURead( uint16_t a ) : address{ a } {}
};


struct CPUWrite
{
  uint16_t address;
  uint8_t value;

  CPUWrite( uint16_t a, uint8_t v ) : address{ a }, value{ v } {}
};


struct CPURequest
{
  enum class Type
  {
    NONE,
    READ_OPCODE,
    READ,
    WRITE,
  } mType;

  uint16_t address;
  uint8_t value;
  uint8_t interrupt;

  CPURequest() : mType{ Type::NONE }, address{}, value{}, interrupt{} {}
  CPURequest( CPURead r ) : mType{ Type::READ }, address{ r.address }, value{}, interrupt{} {}
  CPURequest( CPUReadOpcode r ) : mType{ Type::READ_OPCODE }, address{ r.address }, value{}, interrupt{} {}
  CPURequest( CPUWrite w ) : mType{ Type::WRITE }, address{ w.address }, value{ w.value }, interrupt{} {}

  void resume()
  {
    mType = Type::NONE;
    coro.resume();
  }

  std::experimental::coroutine_handle<> coro;
};

struct AwaitCPURead
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

struct AwaitCPUReadOpcode
{
  CPURequest * req;

  bool await_ready()
  {
    return false;
  }

  OpInt await_resume()
  {
    return { (Opcode)req->value, (int)req->interrupt };
  }

  void await_suspend( std::experimental::coroutine_handle<> c )
  {
    req->coro = c;
  }
};

struct AwaitCPUWrite
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
    AwaitCPURead yield_value( CPURead r );
    AwaitCPUReadOpcode yield_value( CPUReadOpcode r );
    AwaitCPUWrite yield_value( CPUWrite r );

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

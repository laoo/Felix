#pragma once
#include <experimental/coroutine>
#include "CPU.hpp"

class BusMaster;

struct OpInt
{
  uint64_t tick;
  int interrupt;
  Opcode op;
};


struct CPUFetchOpcode
{
  uint16_t address;

  struct Tag {};

  CPUFetchOpcode( uint16_t a, Tag t ) : address{ a } {}
};

struct CPUFetchOperand
{
  uint16_t address;

  struct Tag {};

  CPUFetchOperand( uint16_t a, Tag t ) : address{ a } {}
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
    FETCH_OPCODE,
    FETCH_OPERAND,
    READ,
    WRITE,
  } mType;

  uint64_t tick;
  uint16_t address;
  uint8_t value;
  uint8_t interrupt;

  CPURequest() : mType{ Type::NONE }, address{}, value{}, interrupt{} {}
  CPURequest( CPURead r ) : mType{ Type::READ }, address{ r.address }, value{}, interrupt{} {}
  CPURequest( CPUFetchOpcode r ) : mType{ Type::FETCH_OPCODE }, address{ r.address }, value{}, interrupt{} {}
  CPURequest( CPUFetchOperand r ) : mType{ Type::FETCH_OPERAND }, address{ r.address }, value{}, interrupt{} {}
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

  uint8_t await_resume()
  {
    return req->value;
  }

  void await_suspend( std::experimental::coroutine_handle<> c )
  {
    req->coro = c;
  }
};

struct AwaitCPUFetchOpcode
{
  CPURequest * req;

  bool await_ready()
  {
    return false;
  }

  OpInt await_resume()
  {
    return { req->tick, ( int )req->interrupt, ( Opcode )req->value };
  }

  void await_suspend( std::experimental::coroutine_handle<> c )
  {
    req->coro = c;
  }
};

struct AwaitCPUFetchOperand
{
  CPURequest * req;

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

struct CpuExecute
{
  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;

  struct promise_type
  {
    auto get_return_object() { return CpuExecute{ this, handle::from_promise( *this ) }; }
    auto initial_suspend() { return std::experimental::suspend_always{}; }
    auto final_suspend() noexcept { return std::experimental::suspend_always{}; }
    void return_void() {}
    void unhandled_exception() { std::terminate(); }
    AwaitCPURead yield_value( CPURead r );
    AwaitCPUFetchOpcode yield_value( CPUFetchOpcode r );
    AwaitCPUFetchOperand yield_value( CPUFetchOperand r );
    AwaitCPUWrite yield_value( CPUWrite r );

    BusMaster * mBus;
  };

  CpuExecute() : promise{}, coro{}
  {
  }

  CpuExecute( promise_type * promise, handle c ) : promise{ promise }, coro{ c }
  {
  }

  ~CpuExecute()
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

CpuExecute cpuExecute( CPU & cpu );

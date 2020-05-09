#pragma once
#include <experimental/coroutine>
#include "CPU.hpp"

class BusMaster;

struct COpInt
{
  uint64_t cycle;
  Opcode op;
  int interrupt;
};

struct TraceRequest
{
  uint64_t cycle;
  enum class Type : uint16_t
  {
    NONE,
    FETCH_OPCODE,
    FETCH_OPERAND,
  } mType;

  uint8_t value;
  uint8_t interrupt;

  TraceRequest() : cycle{}, mType { Type::NONE }, value{}, interrupt{} {}

  void resume()
  {
    mType = Type::NONE;
    if ( coro )
      coro.resume();
  }

  std::experimental::coroutine_handle<> coro;
};

struct AwaitDisasmFetchOpcode
{
  TraceRequest & req;

  AwaitDisasmFetchOpcode( TraceRequest & req ) : req{ req } {}

  bool await_ready()
  {
    return false;
  }

  COpInt await_resume()
  {
    return { req.cycle, (Opcode)req.value, req.interrupt };
  }

  void await_suspend( std::experimental::coroutine_handle<> c )
  {
    req.coro = c;
  }
};

struct AwaitDisasmFetchOperand
{
  TraceRequest & req;

  AwaitDisasmFetchOperand( TraceRequest & req ) : req{ req } {}

  bool await_ready()
  {
    return false;
  }

  uint8_t await_resume()
  {
    return req.value;
  }

  void await_suspend( std::experimental::coroutine_handle<> c )
  {
    req.coro = c;
  }
};

struct CpuTrace
{
  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;

  struct promise_type
  {
    auto get_return_object() { return CpuTrace{ handle::from_promise( *this ) }; }
    auto initial_suspend() { return std::experimental::suspend_never{}; }
    auto final_suspend() noexcept { return std::experimental::suspend_always{}; }
    void return_void() {}
    void unhandled_exception() { std::terminate(); }

    BusMaster * mBus;
  };

  CpuTrace( handle c ) : coro{ c }
  {
  }

  ~CpuTrace()
  {
    if ( coro )
      coro.destroy();
  }

  handle coro;
};

CpuTrace cpuTrace( CPU & cpu, TraceRequest & req );

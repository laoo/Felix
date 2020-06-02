#pragma once
#include <experimental/coroutine>
#include "CPU.hpp"

class Felix;

struct TraceRequest
{
  TraceRequest(){}

  void resume()
  {
    if ( coro )
      coro.resume();
  }

  std::experimental::coroutine_handle<> coro;
};

struct AwaitDisasmFetch
{
  TraceRequest & req;

  AwaitDisasmFetch( TraceRequest & req ) : req{ req } {}

  bool await_ready()
  {
    return false;
  }

  void await_resume()
  {
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

    Felix * mBus;
  };

  CpuTrace() : coro{}
  {
  }

  CpuTrace( handle c ) : coro{ c }
  {
  }

  CpuTrace( CpuTrace const & other ) = delete;
  CpuTrace & operator=( CpuTrace const & other ) = delete;
  CpuTrace & operator=( CpuTrace && other ) noexcept
  {
    std::swap( coro, other.coro );
    return *this;
  }


  ~CpuTrace()
  {
    if ( coro )
      coro.destroy();
  }

  handle coro;
};

CpuTrace cpuTrace( CPU & cpu, TraceRequest & req );

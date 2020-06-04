#pragma once
#include <experimental/coroutine>

class Felix;
enum class Opcode : uint8_t;

struct OpInt
{
  uint64_t tick;
  int interrupt;
  Opcode op;
};


struct CPUFetchOpcodeAwaiter
{
  bool await_ready();
  OpInt await_resume();
  void await_suspend( std::experimental::coroutine_handle<> c );
};

struct CPUFetchOperandAwaiter
{
  bool await_ready();
  uint8_t await_resume();
  void await_suspend( std::experimental::coroutine_handle<> c );
};

struct CPUReadAwaiter
{
  bool await_ready();
  uint8_t await_resume();
  void await_suspend( std::experimental::coroutine_handle<> c );
};

struct CPUWriteAwaiter
{
  bool await_ready();
  void await_resume();
  void await_suspend( std::experimental::coroutine_handle<> c );
};


struct CpuExecute
{
  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;

  struct promise_type
  {
    auto get_return_object() { return CpuExecute{ handle::from_promise( *this ) }; }
    auto initial_suspend() { return std::experimental::suspend_never{}; }
    auto final_suspend() noexcept { return std::experimental::suspend_always{}; }
    void return_void() {}
    void unhandled_exception() { std::terminate(); }
  };

  CpuExecute( handle c );
  ~CpuExecute();
};

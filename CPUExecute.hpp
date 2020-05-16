#pragma once
#include <experimental/coroutine>

class BusMaster;
enum class Opcode : uint8_t;

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

  explicit CPUFetchOpcode( uint16_t a ) : address{ a } {}
};

struct CPUFetchOperand
{
  uint16_t address;

  explicit CPUFetchOperand( uint16_t a ) : address{ a } {}
};

struct CPURead
{
  uint16_t address;

  explicit CPURead( uint16_t a ) : address{ a } {}
};


struct CPUWrite
{
  uint16_t address;
  uint8_t value;

  explicit CPUWrite( uint16_t a, uint8_t v ) : address{ a }, value{ v } {}
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

  CPURequest() : mType{ Type::NONE }, tick{}, address{}, value{}, interrupt{} {}
  CPURequest( CPURead r ) : mType{ Type::READ }, tick{}, address{ r.address }, value{}, interrupt{} {}
  CPURequest( CPUFetchOpcode r ) : mType{ Type::FETCH_OPCODE }, tick{}, address{ r.address }, value{}, interrupt{} {}
  CPURequest( CPUFetchOperand r ) : mType{ Type::FETCH_OPERAND }, tick{}, address{ r.address }, value{}, interrupt{} {}
  CPURequest( CPUWrite w ) : mType{ Type::WRITE }, tick{}, address{ w.address }, value{ w.value }, interrupt{} {}

  void operator()()
  {
    mType = Type::NONE;
    coro();
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
    auto get_return_object() { return CpuExecute{ handle::from_promise( *this ) }; }
    auto initial_suspend() { return std::experimental::suspend_always{}; }
    auto final_suspend() noexcept { return std::experimental::suspend_always{}; }
    void return_void() {}
    void unhandled_exception() { std::terminate(); }
    AwaitCPURead await_transform( CPURead r );
    AwaitCPUFetchOpcode await_transform( CPUFetchOpcode r );
    AwaitCPUFetchOperand await_transform( CPUFetchOperand r );
    AwaitCPUWrite await_transform( CPUWrite r );

    BusMaster * mBus;
  };

  CpuExecute() : coro{}
  {
  }

  CpuExecute( handle c ) : coro{ c }
  {
  }

  ~CpuExecute()
  {
    if ( coro )
      coro.destroy();
  }

  void setBusMaster( BusMaster * bus )
  {
    coro.promise().mBus = bus;
    coro();
  }

  handle coro;
};

#pragma once
#include <cstdint>
#include <experimental/coroutine>

enum class Opcode : uint8_t;
class Felix;
struct CpuTrace;
struct TraceRequest;

class CPU
{

  template <class T>
  class NonCopyable
  {
  public:
    NonCopyable( const NonCopyable & ) = delete;
    T & operator = ( const T & ) = delete;

  protected:
    NonCopyable() = default;
    ~NonCopyable() = default;
  };

public:

  struct Request : private NonCopyable<Request>
  {
    enum class Type : uint8_t
    {
      NONE,
      FETCH_OPCODE,
      FETCH_OPERAND,
      READ,
      WRITE,
    };

    uint16_t address;
    uint8_t value;
    Type type;
  };

  struct Response : private NonCopyable<Response>
  {
    uint64_t tick;
    int interrupt;
    uint8_t value;
    std::experimental::coroutine_handle<> target;
  };

  static constexpr int I_NONE  = 0;
  static constexpr int I_IRQ   = 1;
  static constexpr int I_NMI   = 2;
  static constexpr int I_RESET = 4;

  CPU( Felix & felix );

  Request & req();
  Response & res();

private:
  Felix & felix;
  struct OpInt
  {
    uint64_t tick;
    int interrupt;
    Opcode op;
  } opint;

  union
  {
    uint16_t pc;
    struct
    {
      uint8_t pcl;
      uint8_t pch;
    };
  };
  union
  {
    uint16_t s;
    struct
    {
      uint8_t sl;
      uint8_t sh;
    };
  };
  uint8_t a;
  uint8_t x;
  uint8_t y;
  uint8_t P;

  uint8_t operand;



  static constexpr int bitC = 0;
  static constexpr int bitZ = 1;
  static constexpr int bitI = 2;
  static constexpr int bitD = 3;
  static constexpr int bitB = 4;
  static constexpr int bitV = 6;
  static constexpr int bitN = 7;

  uint8_t p() const
  {
    return P | 0x30;
  }

  uint8_t pirq() const
  {
    return P | 0x20;
  }

  void setnz( uint8_t v )
  {
    set<bitZ>( v == 0 );
    set<bitN>( v >= 0x80 );
  }

  void setz( uint8_t v )
  {
    set<bitZ>( v == 0 );
  }

  template<int bit>
  void set()
  {
    P |= 1 << bit;
  }

  template<int bit>
  void set( bool value )
  {
    value ? set<bit>() : clear<bit>();
  }

  template<int bit>
  void clear()
  {
    P &= ~( 1 << bit );
  }

  template<int bit>
  void clear( bool value )
  {
    value ? clear<bit>() : set<bit>();
  }

  template<int bit>
  bool get() const
  {
    return ( P & ( 1 << bit ) ) != 0;
  }

  void asl( uint8_t & val );
  void lsr( uint8_t & val );
  void rol( uint8_t & val );
  void ror( uint8_t & val );
  bool executeCommon( Opcode opcode, uint8_t value );



  Request mReq;
  Response mRes;

  struct Execute
  {
    struct promise_type;
    using handle = std::experimental::coroutine_handle<promise_type>;

    struct promise_type
    {
      auto get_return_object() { return Execute{ handle::from_promise( *this ) }; }
      auto initial_suspend() { return std::experimental::suspend_always{}; }
      auto final_suspend() noexcept { return std::experimental::suspend_always{}; }
      void return_void() {}
      void unhandled_exception() { std::terminate(); }
    };

    Execute();
    Execute( handle c );
    ~Execute();

    handle coro;
  } mEx;

  Execute execute();

  struct CPUFetchOpcodeAwaiter : public Response
  {
    bool await_ready();
    OpInt await_resume();
    void await_suspend( std::experimental::coroutine_handle<> c );
  };

  struct CPUFetchOperandAwaiter : public Response
  {
    bool await_ready();
    uint8_t await_resume();
    void await_suspend( std::experimental::coroutine_handle<> c );
  };

  struct CPUReadAwaiter : public Response
  {
    bool await_ready();
    uint8_t await_resume();
    void await_suspend( std::experimental::coroutine_handle<> c );
  };

  struct CPUWriteAwaiter : public Response
  {
    bool await_ready();
    void await_resume();
    void await_suspend( std::experimental::coroutine_handle<> c );
  };

  CPUFetchOpcodeAwaiter & fetchOpcode( uint16_t address );
  CPUFetchOperandAwaiter & fetchOperand( uint16_t address );
  CPUReadAwaiter & read( uint16_t address );
  CPUWriteAwaiter & write( uint16_t address, uint8_t value );

  friend CpuTrace cpuTrace( CPU & cpu, TraceRequest & req );
};


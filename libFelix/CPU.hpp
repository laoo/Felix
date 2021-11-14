#pragma once

#include "CPUState.hpp"
#include "Utility.hpp"

enum class Opcode : uint8_t;
struct CpuTrace;
struct TraceRequest;
class TraceHelper;

class CPU
{
public:

  struct Request : private NonCopyable
  {
    enum class Type : uint8_t
    {
      FETCH_OPCODE,
      FETCH_OPERAND,
      READ,
      WRITE,
      DISCARD_READ
    };

    uint16_t address;
    uint8_t value;
    Type type;
  };

  struct Response : private NonCopyable
  {
    Response( CPUState & state ) : state{ state }, tick{}, interrupt{}, value{} {}
    CPUState & state;
    uint64_t tick;
    int interrupt;
    uint8_t value;

    bool await_ready() { return false; }
    void await_suspend( std::coroutine_handle<> c ) {}

  };


  CPU();
  ~CPU();

  Request const& advance();

  void respond( uint8_t value );
  void respond( uint64_t tick, uint8_t value );
  void assertInterrupt( int mask );
  void desertInterrupt( int mask );
  int interruptedMask() const;
  void setLog( std::filesystem::path const & path, uint64_t startCycle, std::shared_ptr<TraceHelper> traceHelper );

  CPUState & state();

private:

  CPUState mState;
  uint8_t operand;

  static constexpr size_t ss = sizeof( CPUState );

  static constexpr int bitC = 0;
  static constexpr int bitZ = 1;
  static constexpr int bitI = 2;
  static constexpr int bitD = 3;
  static constexpr int bitB = 4;
  static constexpr int bit1 = 5;
  static constexpr int bitV = 6;
  static constexpr int bitN = 7;

  /*
    https://github.com/spacerace/6502/blob/master/doc/6502-asm-doc/the%20B%20flag%20and%20BRK%20instruction.txt:
    No actual "B" flag exists inside the 6502's processor status register. The B
    flag only exists in the status flag byte pushed to the stack. Naturally,
    when the flags are restored (via PLP or RTI), the B bit is discarded.

    Depending on the means, the B status flag will be pushed to the stack as
    either 0 or 1.

    software instructions BRK & PHP will push the B flag as being 1.
    hardware interrupts IRQ & NMI will push the B flag as being 0.
  */
  uint8_t getP() const
  {
    return mState.p | ( 1 << bit1 ) | ( mState.interrupt != 0 ? 0 : ( 1 << bitB ) );
  }

  void setP( uint8_t value )
  {
    mState.p = value & ~( 1 << bitB );
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
    mState.p |= 1 << bit;
  }

  template<int bit>
  void set( bool value )
  {
    value ? set<bit>() : clear<bit>();
  }

  template<int bit>
  void clear()
  {
    mState.p &= ~( 1 << bit );
  }

  template<int bit>
  void clear( bool value )
  {
    value ? clear<bit>() : set<bit>();
  }

  template<int bit>
  bool get() const
  {
    return ( mState.p & ( 1 << bit ) ) != 0;
  }

  template<int bit>
  static bool get( uint8_t p)
  {
    return ( p & ( 1 << bit ) ) != 0;
  }

  uint8_t inc( uint8_t val );
  uint8_t dec( uint8_t val );
  uint8_t asl( uint8_t val );
  uint8_t lsr( uint8_t val );
  uint8_t rol( uint8_t val );
  uint8_t ror( uint8_t val );
  void adc( uint8_t val );
  void sbc( uint8_t val );
  void bit( uint8_t val );
  void cmp( uint8_t val );
  void cpx( uint8_t val );
  void cpy( uint8_t val );

  struct Execute : private NonCopyable
  {
    struct promise_type;
    using handle = std::coroutine_handle<promise_type>;

    struct promise_type
    {
      auto get_return_object() { return Execute{ handle::from_promise( *this ) }; }
      auto initial_suspend() { return std::suspend_always{}; }
      auto final_suspend() noexcept { return std::suspend_always{}; }
      void return_void() {}
      void unhandled_exception() { std::terminate(); }
    };

    Execute();
    Execute( handle c );
    ~Execute();

    handle coro;
  } const mEx;

  Request mReq;
  Response mRes;
  bool mTrace;
  std::ofstream mFtrace;
  uint64_t mStartCycle;
  std::shared_ptr<TraceHelper> mTraceHelper;

  Execute execute();
  bool isHiccup();


  auto & fetchOpcode( uint16_t address )
  {
    struct CPUFetchOpcodeAwaiter : public Response
    {
      void await_resume()
      {
        state.tick = tick;
        state.interrupt = interrupt;
        state.op = (Opcode)value;
      }
    };

    mReq.type = Request::Type::FETCH_OPCODE;
    mReq.address = address;
    return static_cast<CPUFetchOpcodeAwaiter &>( mRes );
  }

  auto & fetchOperand( uint16_t address )
  {
    struct CPUFetchOperandAwaiter : public Response
    {
      uint8_t await_resume()
      {
        return value;
      }
    };

    mReq.type = Request::Type::FETCH_OPERAND;
    mReq.address = address;
    return static_cast<CPUFetchOperandAwaiter &>( mRes );
  }


  auto & read( uint16_t address )
  {
    struct CPUReadAwaiter : public Response
    {
      uint8_t await_resume()
      {
        return value;
      }
    };

    mReq.type = Request::Type::READ;
    mReq.address = address;
    return static_cast<CPUReadAwaiter &>( mRes );
  }

  auto & write( uint16_t address, uint8_t value )
  {
    struct CPUWriteAwaiter : public Response
    {
      void await_resume()
      {
      }
    };

    mReq.type = Request::Type::WRITE;
    mReq.address = address;
    mReq.value = value;
    return static_cast<CPUWriteAwaiter &>( mRes );
  }

  auto & discardRead( uint16_t address )
  {
    struct CPUDiscardReadAwaiter : public Response
    {
      uint8_t await_resume()
      {
        return 0;
      }
    };

    mReq.type = Request::Type::DISCARD_READ;
    mReq.address = address;
    return static_cast<CPUDiscardReadAwaiter &>( mRes );
  }

  void trace1();
  void trace2();

private:
  std::array<char, 1024> buf;
  int64_t off;
};


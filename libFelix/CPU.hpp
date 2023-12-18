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

  static constexpr uint16_t NMI_VECTOR = 0xfffa;
  static constexpr uint16_t RESET_VECTOR = 0xfffc;
  static constexpr uint16_t IRQ_VECTOR = 0xfffe;


  struct Request : private NonCopyable //won't compile on gcc https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99575
  {
    enum class Type : uint8_t
    {
      FETCH_OPCODE,
      FETCH_OPERAND,
      READ,
      WRITE
    };

    CpuBreakType cpuBreakType;
    uint16_t address;
    uint8_t value;
    Type type;
  };

  struct Response : private NonCopyable //won't compile on gcc https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99575
  {
    Response( CPUState & state ) : state{ state }, interrupt{}, value{} {}
    CPUState & state;
    int interrupt;
    uint8_t value;

    bool await_ready() { return false; }
    void await_suspend( std::coroutine_handle<> c ) {}

  };


  CPU( std::shared_ptr<TraceHelper> traceHelper );
  ~CPU();

  Request const& advance();

  //triggers a break on next instruction boundary on batch end
  void breakNext();
  //triggers a break on next instruction boundary in a response to RunMode::STEP_IN
  void breakOnStepIn();
  //triggers a break if CPU did not go into a subroutine in a response to RunMode::STEP_OVER
  void breakOnStepOver();
  //triggers a break if CPU goes out from a subroutine in a response to RunMode::STEP_OUT
  void breakOnStepOut();
  //triggers a break originated from a trap on next instruction boundary
  void breakFromTrap();
  //clears any step triggers previously set
  void clearBreak();

  void breakOnBrk( bool value );

  void respond( uint8_t value );
  CpuBreakType respondFetchOpcode( uint8_t value );
  void assertInterrupt( int mask );
  void desertInterrupt( int mask );
  int interruptedMask() const;
  void setLog( std::filesystem::path const & path );

  CPUState & state();

  void enableTrace();
  void disableTrace();
  void toggleTrace( bool on );
  void traceNextCount( int count );
  void printStatus( std::span<uint8_t, 3 * 14> text );
  static bool disasmOp( char* out, Opcode op, CPUState* state = nullptr );
  uint8_t disasmOpr( uint8_t const* ram, char* out, int& pc );
  void disassemblyFromPC( uint8_t const* ram, char * out, int columns, int rows );
  void enableHistory( int columns, int rows );
  void disableHistory();
  void copyHistory( std::span<char> out );

private:

  CPUState mState;
  CPUState mPreviousState;

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
  int mTraceNextCount;
  bool mGlobalTrace;
  std::ofstream mFtrace;
  std::shared_ptr<TraceHelper> mTraceHelper;

  Execute execute();
  bool isHiccup();


  auto & fetchOpcode( uint16_t address )
  {
    struct CPUFetchOpcodeAwaiter : public Response
    {
      void await_resume()
      {
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

  void trace1();
  void trace2();
  void setGlobalTrace();

private:

  struct History
  {
    int columns;
    int rows;
    int cursor;
    std::vector<char> data;

    std::span<char> nextRow();
    void copy( std::span<char> out );
  };

  std::array<char, 1024> buf;
  int64_t off;
  std::unique_ptr<History> mHistory;
  std::mutex mHistoryMutex;
  std::atomic_bool mHistoryPresent;
  //true if mStackBreakCondition is valid for CpuBreakType::STEP_OUT
  bool mPostponedStepOut;
  uint16_t mStackBreakCondition;
  bool mBreakOnBrk;
};


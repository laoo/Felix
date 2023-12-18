#include "pch.hpp"
#include "CPU.hpp"
#include "Opcodes.hpp"
#include "TraceHelper.hpp"
#include "DebugRAM.hpp"
#include <stdarg.h>

namespace
{
static constexpr uint8_t hexTab[16] =
{
  '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
};

char * da_sprintf (char *dst, const char *fmt, ...)
{
  char c, f;
  char fill;
  char *p;
  int width;
  char h[16];
  unsigned long n;
  va_list arg;
  char *dst0;

  dst0 = dst;
  va_start (arg, fmt);

  while ((c = *fmt++))
  {
    if (c != '%')
    {
      if ( c == '\t' )
      {
        for(int column = (int)(dst - dst0); column < 15; ++column)
        {
          *dst++ = ' ';
        }
      }
      else
      {
        *dst++ = c;
      }
      continue;
    }
    c = *fmt++;
    fill = ' ';
    if (c == '0')
    {
      fill = c;
      c = *fmt++;
    }
    for (width = 0; '0' <= c && c <= '9'; c = *fmt++)
    {
      width *= 10;
      width += c - '0';
    }
    width = (width > 15) ? 15 : width;
    p = h;
    *p = '\0';
    f = 0;
    if (c == 'l' || c == 'L')
    {
      f = 1;
      c = *fmt++;
    }
    switch (c)
    {
    case 'x':
    case 'X':
    case 'p':
      if (f)
      {
        n = va_arg (arg, unsigned long);
      }
      else
      {
        n = (unsigned long) va_arg (arg, unsigned int);
      }
      do
      {
        *p++ = hexTab[(int)(n & 0xf)];
        n >>= 4;
      } while (n);
      break;
    case 'd':
    case 'u':
      if (f)
      {
        n = va_arg (arg, unsigned long);
      }
      else
      {
        n = (unsigned long) va_arg (arg, unsigned int);
      }
      if (c == 'd' && (signed long) n < 0)
      {
        *dst++ = '-';
        n = (unsigned long) (-(signed long) n);
      }
      do
      {
        *p++ = hexTab[(int)(n % 10)];
        n /= 10;
      } while (n);
      break;
    case 'c':
      c = va_arg (arg, int);
      *p++ = c;
      break;
    case 's':
      p = va_arg (arg, char *);
      if ( p )
      {
        for (width -= (int)strlen (p); width > 0; --width)
        {
          *dst++ = fill;
        }
        while (*p)
        {
            *dst++ = *p++;
        }
      }
      else
      {
        *dst++ = '(';
        *dst++ = 'N';
        *dst++ = 'I';
        *dst++ = 'L';
        *dst++ = ')';
      }
      continue;
    default:
      /*  empty */
      break;
    }
    for (width = width - (int)(p - h); width > 0; --width)
    {
      *dst++ = fill;
    }
    for (--p; p >= h; --p)
    {
      *dst++ = *p;
    }
  }
  return dst;
}

template<size_t N>
consteval uint16_t l2( char const ( &t )[N] )
{
  int result;
  if constexpr ( N > 0 )
    result = (int)t[0];
  if constexpr ( N > 1 )
    result |= (int)t[1] << 8;
  if constexpr ( N > 2 )
    result |= (int)t[2] << 16;
  if constexpr ( N > 3 )
    result |= (int)t[3] << 24;

  return result;
}

template<size_t N>
consteval uint32_t l4( char const ( &t )[N] )
{
  int result;
  if constexpr ( N > 0 )
    result = (int)t[0];
  if constexpr ( N > 1 )
    result |= (int)t[1] << 8;
  if constexpr ( N > 2 )
    result |= (int)t[2] << 16;
  if constexpr ( N > 3 )
    result |= (int)t[3] << 24;

  return result;
}

}

bool CPU::isHiccup()
{
  switch ( mState.op )
  {
  case Opcode::UND_1_03:
  case Opcode::UND_1_13:
  case Opcode::UND_1_23:
  case Opcode::UND_1_33:
  case Opcode::UND_1_43:
  case Opcode::UND_1_53:
  case Opcode::UND_1_63:
  case Opcode::UND_1_73:
  case Opcode::UND_1_83:
  case Opcode::UND_1_93:
  case Opcode::UND_1_a3:
  case Opcode::UND_1_b3:
  case Opcode::UND_1_c3:
  case Opcode::UND_1_d3:
  case Opcode::UND_1_e3:
  case Opcode::UND_1_f3:
  case Opcode::UND_1_0b:
  case Opcode::UND_1_1b:
  case Opcode::UND_1_2b:
  case Opcode::UND_1_3b:
  case Opcode::UND_1_4b:
  case Opcode::UND_1_5b:
  case Opcode::UND_1_6b:
  case Opcode::UND_1_7b:
  case Opcode::UND_1_8b:
  case Opcode::UND_1_9b:
  case Opcode::UND_1_ab:
  case Opcode::UND_1_bb:
  case Opcode::UND_1_cb:
  case Opcode::UND_1_db:
  case Opcode::UND_1_eb:
  case Opcode::UND_1_fb:
    trace2();
    return true;
  default:
    return false;
  }
}

void CPU::setLog( std::filesystem::path const & path )
{
  mFtrace = std::ofstream{ path };
}

CPUState & CPU::state()
{
  return mState;
}

CPU::CPU( std::shared_ptr<TraceHelper> traceHelper ) : mState{ CPUState::reset() }, mEx{ execute() }, mReq{}, mRes{ mState }, mTrace{}, mTraceNextCount{}, mGlobalTrace{}, mFtrace{}, mTraceHelper{ std::move( traceHelper ) }, mHistory{}, mHistoryPresent{}, off{},
  mPostponedStepOut{}, mStackBreakCondition{ 0xffff }, mBreakOnBrk{ false }
{
  static constexpr char prototype[] = "PC:ffff A:ff X:ff Y:ff S:1ff P=NVDIZC ";
  memcpy( &buf[0], prototype, sizeof prototype );
}

CPU::~CPU()
{
}

CPU::Request const& CPU::advance()
{
  mEx.coro();
  return mReq;
}

void CPU::breakNext()
{
  mReq.cpuBreakType = CpuBreakType::NEXT;
}

void CPU::breakOnStepIn()
{
  mReq.cpuBreakType = CpuBreakType::STEP_IN;
}

void CPU::breakOnStepOver()
{
  mReq.cpuBreakType = CpuBreakType::STEP_OVER;
}

void CPU::breakOnStepOut()
{
  mReq.cpuBreakType = CpuBreakType::NONE;
  mPostponedStepOut = true;
  if ( mStackBreakCondition == 0xffff )
    mStackBreakCondition = mState.s;
}

void CPU::breakFromTrap()
{
  mReq.cpuBreakType = CpuBreakType::TRAP_BREAK;
}

void CPU::clearBreak()
{
  mReq.cpuBreakType = CpuBreakType::NONE;
  mStackBreakCondition = 0xffff;
}

void CPU::breakOnBrk( bool value )
{
  mBreakOnBrk = value;
}

void CPU::respond( uint8_t value )
{
  mRes.value = value;
}

CpuBreakType CPU::respondFetchOpcode( uint8_t value )
{
  mRes.value = value;
  return mReq.cpuBreakType;
}

void CPU::assertInterrupt( int mask )
{
  mRes.interrupt |= mask;
}

int CPU::interruptedMask() const
{
  return mRes.interrupt;
}

void CPU::desertInterrupt( int mask )
{
  mRes.interrupt &= ~mask;
}

CPU::Execute::Execute() : coro{}
{
}

CPU::Execute::Execute( handle c ) : coro{ c }
{
}

CPU::Execute::~Execute()
{
  if ( coro )
  {
    coro.destroy();
  }
}

CPU::Execute CPU::execute()
{
  auto& state = mState;
  mPreviousState = state;

  trace1();



  for ( ;; )
  {
    state.ea = 0;
    state.t = 0;

    if ( state.interrupt && ( !state.i || ( state.interrupt & ~CPUState::I_IRQ ) != 0 ) )
    {
      state.op = Opcode::BRK_BRK;
    }

    state.eal = co_await fetchOperand( state.pc );

    switch ( state.op )
    {
    case Opcode::RZP_AND:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.setnz( state.a &= state.m1 );
      break;
    case Opcode::RZP_BIT:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.bit( state.m1 );
      break;
    case Opcode::RZP_CMP:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.cmp( state.m1 );
      break;
    case Opcode::RZP_CPX:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.cpx( state.m1 );
      break;
    case Opcode::RZP_CPY:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.cpy( state.m1 );
      break;
    case Opcode::RZP_EOR:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.setnz( state.a ^= state.m1 );
      break;
    case Opcode::RZP_LDA:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.setnz( state.a = state.m1 );
      break;
    case Opcode::RZP_LDX:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.setnz( state.x = state.m1 );
      break;
    case Opcode::RZP_LDY:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.setnz( state.y = state.m1 );
      break;
    case Opcode::RZP_ORA:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.setnz( state.a |= state.m1 );
      break;
    case Opcode::RZP_ADC:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.adc( state.m1 );
      if ( state.d )
      {
        co_await read( state.ea );
      }
      break;
    case Opcode::RZP_SBC:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.sbc( state.m1 );
      if ( state.d )
      {
        co_await read( state.ea );
      }
      break;
    case Opcode::WZP_STA:
      ++state.pc;
      co_await write( state.ea, state.a );
      break;
    case Opcode::WZP_STX:
      ++state.pc;
      co_await write( state.ea, state.x );
      break;
    case Opcode::WZP_STY:
      ++state.pc;
      co_await write( state.ea, state.y );
      break;
    case Opcode::WZP_STZ:
      ++state.pc;
      co_await write( state.ea, 0x00 );
      break;
    case Opcode::MZP_ASL:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.asl( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_DEC:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.dec( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_INC:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.inc( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_LSR:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.lsr( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_ROL:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.rol( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_ROR:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.ror( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_TRB:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.setz( state.m1 & state.a );
      state.m2 = state.m1 & ~state.a;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_TSB:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.setz( state.m1 & state.a );
      state.m2 = state.m1 | state.a;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_RMB0:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.m1 & ~0x01;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_RMB1:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.m1 & ~0x02;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_RMB2:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.m1 & ~0x04;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_RMB3:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.m1 & ~0x08;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_RMB4:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.m1 & ~0x10;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_RMB5:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.m1 & ~0x20;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_RMB6:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.m1 & ~0x40;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_RMB7:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.m1 & ~0x80;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_SMB0:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.m1 | 0x01;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_SMB1:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.m1 | 0x02;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_SMB2:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.m1 | 0x04;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_SMB3:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.m1 | 0x08;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_SMB4:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.m1 | 0x10;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_SMB5:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.m1 | 0x20;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_SMB6:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.m1 | 0x40;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_SMB7:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.m1 | 0x80;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::RZX_AND:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      state.setnz( state.a &= state.m1 );
      break;
    case Opcode::RZX_BIT:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      state.bit( state.m1 );
      break;
    case Opcode::RZX_CMP:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      state.cmp( state.m1 );
      break;
    case Opcode::RZX_EOR:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      state.setnz( state.a ^= state.m1 );
      break;
    case Opcode::RZX_LDA:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      state.setnz( state.a = state.m1 );
      break;
    case Opcode::RZX_LDY:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      state.setnz( state.y = state.m1 );
      break;
    case Opcode::RZX_ORA:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      state.setnz( state.a |= state.m1 );
      break;
    case Opcode::RZX_ADC:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      state.adc( state.m1 );
      if ( state.d )
      {
        co_await read( state.t );
      }
      break;
    case Opcode::RZX_SBC:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      state.sbc( state.m1 );
      if ( state.d )
      {
        co_await read( state.t );
      }
      break;
    case Opcode::RZY_LDX:
      co_await read( ++state.pc );
      state.tl = state.eal + state.y;
      state.m1 = co_await read( state.t );
      state.setnz( state.x = state.m1 );
      break;
    case Opcode::WZX_STA:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      co_await write( state.t, state.a );
      break;
    case Opcode::WZX_STY:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      co_await write( state.t, state.y );
      break;
    case Opcode::WZX_STZ:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      co_await write( state.t, 0x00 );
      break;
    case Opcode::WZY_STX:
      co_await read( ++state.pc );
      state.tl = state.eal + state.y;
      co_await write( state.t, state.x );
      break;
    case Opcode::MZX_ASL:
      co_await read( state.pc++ );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      co_await read( state.t );
      state.m2 = state.asl( state.m1 );
      co_await write( state.t, state.m2 );
      break;
    case Opcode::MZX_DEC:
      co_await read( state.pc++ );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      co_await read( state.t );
      state.m2 = state.dec( state.m1 );
      co_await write( state.t, state.m2 );
      break;
    case Opcode::MZX_INC:
      co_await read( state.pc++ );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      co_await read( state.t );
      state.m2 = state.inc( state.m1 );
      co_await write( state.t, state.m2 );
      break;
    case Opcode::MZX_LSR:
      co_await read( state.pc++ );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      co_await read( state.t );
      state.m2 = state.lsr( state.m1 );
      co_await write( state.t, state.m2 );
      break;
    case Opcode::MZX_ROL:
      co_await read( state.pc++ );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      co_await read( state.t );
      state.m2 = state.rol( state.m1 );
      co_await write( state.t, state.m2 );
      break;
    case Opcode::MZX_ROR:
      co_await read( state.pc++ );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      co_await read( state.t );
      state.m2 = state.ror( state.m1 );
      co_await write( state.t, state.m2 );
      break;
    case Opcode::RIN_AND:
      ++state.pc;
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.setnz( state.a &= state.m1 );
      break;
    case Opcode::RIN_CMP:
      ++state.pc;
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.cmp( state.m1 );
      break;
    case Opcode::RIN_EOR:
      ++state.pc;
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.setnz( state.a ^= state.m1 );
      break;
    case Opcode::RIN_LDA:
      ++state.pc;
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.setnz( state.a = state.m1 );
      break;
    case Opcode::RIN_ORA:
      ++state.pc;
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.setnz( state.a |= state.m1 );
      break;
    case Opcode::RIN_ADC:
      ++state.pc;
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.adc( state.m1 );
      if ( state.d )
      {
        co_await read( state.t );
      }
      break;
    case Opcode::RIN_SBC:
      ++state.pc;
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.sbc( state.m1 );
      if ( state.d )
      {
        co_await read( state.t );
      }
      break;
    case Opcode::WIN_STA:
      ++state.pc;
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      co_await write( state.t, state.a );
      break;
    case Opcode::RIX_AND:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.eal += state.x;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.setnz( state.a &= state.m1 );
      break;
    case Opcode::RIX_CMP:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.eal += state.x;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.cmp( state.m1 );
      break;
    case Opcode::RIX_EOR:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.eal += state.x;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.setnz( state.a ^= state.m1 );
      break;
    case Opcode::RIX_LDA:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.eal += state.x;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.setnz( state.a = state.m1 );
      break;
    case Opcode::RIX_ORA:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.eal += state.x;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.setnz( state.a |= state.m1 );
      break;
    case Opcode::RIX_ADC:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.eal += state.x;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.adc( state.m1 );
      if ( state.d )
      {
        co_await read( state.t );
      }
      break;
    case Opcode::RIX_SBC:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.eal += state.x;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.sbc( state.m1 );
      if ( state.d )
      {
        co_await read( state.t );
      }
      break;
    case Opcode::WIX_STA:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.eal += state.x;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      co_await write( state.t, state.a );
      break;
    case Opcode::RIY_AND:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.ea = state.t;
      state.ea += state.y;
      if ( state.eah != state.th )
      {
        state.tl += state.y;
        co_await read( state.t );
      }
      state.m1 = co_await read( state.ea );
      state.setnz( state.a &= state.m1 );
      break;
    case Opcode::RIY_CMP:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.ea = state.t;
      state.ea += state.y;
      if ( state.eah != state.th )
      {
        state.tl += state.y;
        co_await read( state.t );
      }
      state.m1 = co_await read( state.ea );
      state.cmp( state.m1 );
      break;
    case Opcode::RIY_EOR:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.ea = state.t;
      state.ea += state.y;
      if ( state.eah != state.th )
      {
        state.tl += state.y;
        co_await read( state.t );
      }
      state.m1 = co_await read( state.ea );
      state.setnz( state.a ^= state.m1 );
      break;
    case Opcode::RIY_LDA:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.ea = state.t;
      state.ea += state.y;
      if ( state.eah != state.th )
      {
        state.tl += state.y;
        co_await read( state.t );
      }
      state.m1 = co_await read( state.ea );
      state.setnz( state.a = state.m1 );
      break;
    case Opcode::RIY_ORA:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.ea = state.t;
      state.ea += state.y;
      if ( state.eah != state.th )
      {
        state.tl += state.y;
        co_await read( state.t );
      }
      state.m1 = co_await read( state.ea );
      state.setnz( state.a |= state.m1 );
      break;
    case Opcode::RIY_ADC:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.ea = state.t;
      state.ea += state.y;
      if ( state.eah != state.th )
      {
        state.tl += state.y;
        co_await read( state.t );
      }
      state.m1 = co_await read( state.ea );
      state.adc( state.m1 );
      if ( state.d )
      {
        co_await read( state.t );
      }
      break;
    case Opcode::RIY_SBC:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.ea = state.t;
      state.ea += state.y;
      if ( state.eah != state.th )
      {
        state.tl += state.y;
        co_await read( state.t );
      }
      state.m1 = co_await read( state.ea );
      state.sbc( state.m1 );
      if ( state.d )
      {
        co_await read( state.t );
      }
      break;
    case Opcode::WIY_STA:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.ea = state.t;
      state.ea += state.y;
      state.tl += state.y;
      co_await read( state.t );
      co_await write( state.ea, state.a );
      break;
    case Opcode::RAB_AND:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      state.setnz( state.a &= state.m1 );
      break;
    case Opcode::RAB_BIT:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      state.bit( state.m1 );
      break;
    case Opcode::RAB_CMP:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      state.cmp( state.m1 );
      break;
    case Opcode::RAB_CPX:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      state.cpx( state.m1 );
      break;
    case Opcode::RAB_CPY:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      state.cpy( state.m1 );
      break;
    case Opcode::RAB_EOR:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      state.setnz( state.a ^= state.m1 );
      break;
    case Opcode::RAB_LDA:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      state.setnz( state.a = state.m1 );
      break;
    case Opcode::RAB_LDX:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      state.setnz( state.x = state.m1 );
      break;
    case Opcode::RAB_LDY:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      state.setnz( state.y = state.m1 );
      break;
    case Opcode::RAB_ORA:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      state.setnz( state.a |= state.m1 );
      break;
    case Opcode::RAB_ADC:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      state.adc( state.m1 );
      if ( state.d )
      {
        co_await read( state.ea );
      }
      break;
    case Opcode::RAB_SBC:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      state.sbc( state.m1 );
      if ( state.d )
      {
        co_await read( state.ea );
      }
      break;
    case Opcode::WAB_STA:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      co_await write( state.ea, state.a );
      break;
    case Opcode::WAB_STX:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      co_await write( state.ea, state.x );
      break;
    case Opcode::WAB_STY:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      co_await write( state.ea, state.y );
      break;
    case Opcode::WAB_STZ:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      co_await write( state.ea, 0x00 );
      break;
    case Opcode::MAB_ASL:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.asl( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAB_DEC:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.dec( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAB_INC:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.inc( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAB_LSR:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.lsr( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAB_ROL:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.rol( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAB_ROR:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.m2 = state.ror( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAB_TRB:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.setz( state.m1 & state.a );
      state.m2 = state.m1 & ~state.a;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAB_TSB:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      co_await read( state.ea );
      state.setz( state.m1 & state.a );
      state.m2 = state.m1 | state.a;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::RAX_AND:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.x;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      state.setnz( state.a &= state.m1 );
      state.pc += 2;
      break;
    case Opcode::RAX_BIT:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.x;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      state.bit( state.m1 );
      state.pc += 2;
      break;
    case Opcode::RAX_CMP:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.x;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      state.cmp( state.m1 );
      state.pc += 2;
      break;
    case Opcode::RAX_EOR:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.x;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      state.setnz( state.a ^= state.m1 );
      state.pc += 2;
      break;
    case Opcode::RAX_LDA:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.x;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      state.setnz( state.a = state.m1 );
      state.pc += 2;
      break;
    case Opcode::RAX_LDY:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.x;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      state.setnz( state.y = state.m1 );
      state.pc += 2;
      break;
    case Opcode::RAX_ORA:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.x;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      state.setnz( state.a |= state.m1 );
      state.pc += 2;
      break;
    case Opcode::RAX_ADC:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.x;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      state.adc( state.m1 );
      if ( state.d )
      {
        co_await read( state.fa );
      }
      state.pc += 2;
      break;
    case Opcode::RAX_SBC:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.x;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      state.sbc( state.m1 );
      if ( state.d )
      {
        co_await read( state.fa );
      }
      state.pc += 2;
      break;
    case Opcode::RAY_AND:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.y;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      state.setnz( state.a &= state.m1 );
      state.pc += 2;
      break;
    case Opcode::RAY_CMP:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.y;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      state.cmp( state.m1 );
      state.pc += 2;
      break;
    case Opcode::RAY_EOR:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.y;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      state.setnz( state.a ^= state.m1 );
      state.pc += 2;
      break;
    case Opcode::RAY_LDA:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.y;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      state.setnz( state.a = state.m1 );
      state.pc += 2;
      break;
    case Opcode::RAY_LDX:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.y;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      state.setnz( state.x = state.m1 );
      state.pc += 2;
      break;
    case Opcode::RAY_ORA:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.y;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      state.setnz( state.a |= state.m1 );
      state.pc += 2;
      break;
    case Opcode::RAY_ADC:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.y;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      state.adc( state.m1 );
      if ( state.d )
      {
        co_await read( state.ea );
      }
      state.pc += 2;
      break;
    case Opcode::RAY_SBC:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.y;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      state.sbc( state.m1 );
      if ( state.d )
      {
        co_await read( state.ea );
      }
      state.pc += 2;
      break;
    case Opcode::WAX_STA:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.x;
      co_await read( state.pc + 1 );
      co_await write( state.fa, state.a );
      state.pc += 2;
      break;
    case Opcode::WAX_STZ:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.x;
      co_await read( state.pc + 1 );
      co_await write( state.fa, 0x00 );
      state.pc += 2;
      break;
    case Opcode::WAY_STA:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.y;
      co_await read( state.pc + 1 );
      co_await write( state.fa, state.a );
      state.pc += 2;
      break;
    case Opcode::MAX_ASL:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.x;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      co_await read( state.fa );
      state.m2 = state.asl( state.m1 );
      co_await write( state.fa, state.m2 );
      state.pc += 2;
      break;
    case Opcode::MAX_DEC:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.x;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      co_await read( state.fa );
      state.m2 = state.dec( state.m1 );
      co_await write( state.fa, state.m2 );
      state.pc += 2;
      break;
    case Opcode::MAX_INC:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.x;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      co_await read( state.fa );
      state.m2 = state.inc( state.m1 );
      co_await write( state.fa, state.m2 );
      state.pc += 2;
      break;
    case Opcode::MAX_LSR:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.x;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      co_await read( state.fa );
      state.m2 = state.lsr( state.m1 );
      co_await write( state.fa, state.m2 );
      state.pc += 2;
      break;
    case Opcode::MAX_ROL:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.x;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      co_await read( state.fa );
      state.m2 = state.rol( state.m1 );
      co_await write( state.fa, state.m2 );
      state.pc += 2;
      break;
    case Opcode::MAX_ROR:
      state.eah = co_await fetchOperand( state.pc + 1 );
      state.fa = state.ea + state.x;
      if ( state.eah != state.fah )
      {
        co_await read( state.pc + 1 );
      }
      state.m1 = co_await read( state.fa );
      co_await read( state.fa );
      state.m2 = state.ror( state.m1 );
      co_await write( state.fa, state.m2 );
      state.pc += 2;
      break;
    case Opcode::JMA_JMP:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.pc = state.ea;
      break;
    case Opcode::JSA_JSR:
      ++state.pc;
      co_await read( state.s );
      co_await write( state.s, state.pch );
      state.sl--;
      co_await write( state.s, state.pcl );
      state.sl--;
      state.eah = co_await fetchOperand( state.pc++ );
      state.pc = state.ea;
      if ( mReq.cpuBreakType == CpuBreakType::STEP_OVER )
      {
        mReq.cpuBreakType = CpuBreakType::NONE;
        mStackBreakCondition = state.s;
      }
      break;
    case Opcode::JMX_JMP:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      co_await read( state.pc );
      state.fa = state.t = state.ea;
      state.eal += state.x;
      co_await read( state.ea );
      state.t += state.x;
      state.eal = co_await read( state.t++ );
      state.eah = co_await read( state.t );
      state.pc = state.ea;
      break;
    case Opcode::JMI_JMP:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.tl = co_await read( state.ea );
      state.eal++;
      co_await read( state.ea );
      state.eah += state.eal == 0 ? 1 : 0;
      state.th = co_await read( state.ea );
      state.pc = state.t;
      break;
    case Opcode::IMP_ASL:
      state.a = state.asl( state.a );
      break;
    case Opcode::IMP_CLC:
      state.c.clear();
      break;
    case Opcode::IMP_CLD:
      state.d.clear();
      break;
    case Opcode::IMP_CLI:
      state.i.clear();
      break;
    case Opcode::IMP_CLV:
      state.v.clear();
      break;
    case Opcode::IMP_DEC:
      state.a = state.dec( state.a );
      break;
    case Opcode::IMP_DEX:
      state.x = state.dec( state.x );
      break;
    case Opcode::IMP_DEY:
      state.y = state.dec( state.y );
      break;
    case Opcode::IMP_INC:
      state.a = state.inc( state.a );
      break;
    case Opcode::IMP_INX:
      state.x = state.inc( state.x );
      break;
    case Opcode::IMP_INY:
      state.y = state.inc( state.y );
      break;
    case Opcode::IMP_LSR:
      state.a = state.lsr( state.a );
      break;
    case Opcode::IMP_NOP:
      break;
    case Opcode::IMP_ROL:
      state.a = state.rol( state.a );
      break;
    case Opcode::IMP_ROR:
      state.a = state.ror( state.a );
      break;
    case Opcode::IMP_SEC:
      state.c.set();
      break;
    case Opcode::IMP_SED:
      state.d.set();
      break;
    case Opcode::IMP_SEI:
      state.i.set();
      break;
    case Opcode::IMP_TAX:
      state.setnz( state.x = state.a );
      break;
    case Opcode::IMP_TAY:
      state.setnz( state.y = state.a );
      break;
    case Opcode::IMP_TSX:
      state.setnz( state.x = state.sl );
      break;
    case Opcode::IMP_TXA:
      state.setnz( state.a = state.x );
      break;
    case Opcode::IMP_TXS:
      state.sl = state.x;
      break;
    case Opcode::IMP_TYA:
      state.setnz( state.a = state.y );
      break;
    case Opcode::IMM_AND:
      ++state.pc;
      state.setnz( state.a &= state.eal );
      break;
    case Opcode::IMM_BIT:
      ++state.pc;
      state.setz( state.a & state.eal );
      break;
    case Opcode::IMM_CMP:
      ++state.pc;
      state.cmp( state.eal );
      break;
    case Opcode::IMM_CPX:
      ++state.pc;
      state.cpx( state.eal );
      break;
    case Opcode::IMM_CPY:
      ++state.pc;
      state.cpy( state.eal );
      break;
    case Opcode::IMM_EOR:
      ++state.pc;
      state.setnz( state.a ^= state.eal );
      break;
    case Opcode::IMM_LDA:
      ++state.pc;
      state.setnz( state.a = state.eal );
      break;
    case Opcode::IMM_LDX:
      ++state.pc;
      state.setnz( state.x = state.eal );
      break;
    case Opcode::IMM_LDY:
      ++state.pc;
      state.setnz( state.y = state.eal );
      break;
    case Opcode::IMM_ORA:
      ++state.pc;
      state.setnz( state.a |= state.eal );
      break;
    case Opcode::IMM_ADC:
      ++state.pc;
      state.adc( state.eal );
      if ( state.d )
      {
        co_await read( state.pc );
      }
      break;
    case Opcode::IMM_SBC:
      ++state.pc;
      state.sbc( state.eal );
      if ( state.d )
      {
        co_await read( state.pc );
      }
      break;
    case Opcode::BRL_BCC:
      ++state.pc;
      state.t = state.pc + ( int8_t )state.eal;
      if ( !state.c )
      {
        co_await read( state.pc );
        if ( state.th != state.pch )
        {
          co_await read( state.pc );
        }
        state.pc = state.t;
      }
      break;
    case Opcode::BRL_BCS:
      ++state.pc;
      state.t = state.pc + ( int8_t )state.eal;
      if ( state.c )
      {
        co_await read( state.pc );
        if ( state.th != state.pch )
        {
          co_await read( state.pc );
        }
        state.pc = state.t;
      }
      break;
    case Opcode::BRL_BEQ:
      ++state.pc;
      state.t = state.pc + ( int8_t )state.eal;
      if ( state.z )
      {
        co_await read( state.pc );
        if ( state.th != state.pch )
        {
          co_await read( state.pc );
        }
        state.pc = state.t;
      }
      break;
    case Opcode::BRL_BMI:
      ++state.pc;
      state.t = state.pc + ( int8_t )state.eal;
      if ( state.n )
      {
        co_await read( state.pc );
        if ( state.th != state.pch )
        {
          co_await read( state.pc );
        }
        state.pc = state.t;
      }
      break;
    case Opcode::BRL_BNE:
      ++state.pc;
      state.t = state.pc + ( int8_t )state.eal;
      if ( !state.z )
      {
        co_await read( state.pc );
        if ( state.th != state.pch )
        {
          co_await read( state.pc );
        }
        state.pc = state.t;
      }
      break;
    case Opcode::BRL_BPL:
      ++state.pc;
      state.t = state.pc + ( int8_t )state.eal;
      if ( !state.n )
      {
        co_await read( state.pc );
        if ( state.th != state.pch )
        {
          co_await read( state.pc );
        }
        state.pc = state.t;
      }
      break;
    case Opcode::BRL_BRA:
      co_await read( ++state.pc );
      state.t = state.pc + ( int8_t )state.eal;
      if ( state.th != state.pch )
      {
        co_await read( state.pc );
      }
      state.pc = state.t;
      break;
    case Opcode::BRL_BVC:
      ++state.pc;
      state.t = state.pc + ( int8_t )state.eal;
      if ( !state.v )
      {
        co_await read( state.pc );
        if ( state.th != state.pch )
        {
          co_await read( state.pc );
        }
        state.pc = state.t;
      }
      break;
    case Opcode::BRL_BVS:
      ++state.pc;
      state.t = state.pc + ( int8_t )state.eal;
      if ( state.v )
      {
        co_await read( state.pc );
        if ( state.th != state.pch )
        {
          co_await read( state.pc );
        }
        state.pc = state.t;
      }
      break;
    case Opcode::BZR_BBR0:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.tl = co_await fetchOperand( state.pc++ );
      co_await read( state.ea );
      state.t = state.pc + ( int8_t )state.tl;
      if ( ( state.m1 & 0x01 ) == 0 )
      {
        state.pc = state.t;
      }
      break;
    case Opcode::BZR_BBR1:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.tl = co_await fetchOperand( state.pc++ );
      co_await read( state.ea );
      state.t = state.pc + ( int8_t )state.tl;
      if ( ( state.m1 & 0x02 ) == 0 )
      {
        state.pc = state.t;
      }
      break;
    case Opcode::BZR_BBR2:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.tl = co_await fetchOperand( state.pc++ );
      co_await read( state.ea );
      state.t = state.pc + ( int8_t )state.tl;
      if ( ( state.m1 & 0x04 ) == 0 )
      {
        state.pc = state.t;
      }
      break;
    case Opcode::BZR_BBR3:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.tl = co_await fetchOperand( state.pc++ );
      co_await read( state.ea );
      state.t = state.pc + ( int8_t )state.tl;
      if ( ( state.m1 & 0x08 ) == 0 )
      {
        state.pc = state.t;
      }
      break;
    case Opcode::BZR_BBR4:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.tl = co_await fetchOperand( state.pc++ );
      co_await read( state.ea );
      state.t = state.pc + ( int8_t )state.tl;
      if ( ( state.m1 & 0x10 ) == 0 )
      {
        state.pc = state.t;
      }
      break;
    case Opcode::BZR_BBR5:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.tl = co_await fetchOperand( state.pc++ );
      co_await read( state.ea );
      state.t = state.pc + ( int8_t )state.tl;
      if ( ( state.m1 & 0x20 ) == 0 )
      {
        state.pc = state.t;
      }
      break;
    case Opcode::BZR_BBR6:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.tl = co_await fetchOperand( state.pc++ );
      co_await read( state.ea );
      state.t = state.pc + ( int8_t )state.tl;
      if ( ( state.m1 & 0x40 ) == 0 )
      {
        state.pc = state.t;
      }
      break;
    case Opcode::BZR_BBR7:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.tl = co_await fetchOperand( state.pc++ );
      co_await read( state.ea );
      state.t = state.pc + ( int8_t )state.tl;
      if ( ( state.m1 & 0x80 ) == 0 )
      {
        state.pc = state.t;
      }
      break;
    case Opcode::BZR_BBS0:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.tl = co_await fetchOperand( state.pc++ );
      co_await read( state.ea );
      state.t = state.pc + ( int8_t )state.tl;
      if ( ( state.m1 & 0x01 ) != 0 )
      {
        state.pc = state.t;
      }
      break;
    case Opcode::BZR_BBS1:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.tl = co_await fetchOperand( state.pc++ );
      co_await read( state.ea );
      state.t = state.pc + ( int8_t )state.tl;
      if ( ( state.m1 & 0x02 ) != 0 )
      {
        state.pc = state.t;
      }
      break;
    case Opcode::BZR_BBS2:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.tl = co_await fetchOperand( state.pc++ );
      co_await read( state.ea );
      state.t = state.pc + ( int8_t )state.tl;
      if ( ( state.m1 & 0x04 ) != 0 )
      {
        state.pc = state.t;
      }
      break;
    case Opcode::BZR_BBS3:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.tl = co_await fetchOperand( state.pc++ );
      co_await read( state.ea );
      state.t = state.pc + ( int8_t )state.tl;
      if ( ( state.m1 & 0x08 ) != 0 )
      {
        state.pc = state.t;
      }
      break;
    case Opcode::BZR_BBS4:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.tl = co_await fetchOperand( state.pc++ );
      co_await read( state.ea );
      state.t = state.pc + ( int8_t )state.tl;
      if ( ( state.m1 & 0x10 ) != 0 )
      {
        state.pc = state.t;
      }
      break;
    case Opcode::BZR_BBS5:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.tl = co_await fetchOperand( state.pc++ );
      co_await read( state.ea );
      state.t = state.pc + ( int8_t )state.tl;
      if ( ( state.m1 & 0x20 ) != 0 )
      {
        state.pc = state.t;
      }
      break;
    case Opcode::BZR_BBS6:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.tl = co_await fetchOperand( state.pc++ );
      co_await read( state.ea );
      state.t = state.pc + ( int8_t )state.tl;
      if ( ( state.m1 & 0x40 ) != 0 )
      {
        state.pc = state.t;
      }
      break;
    case Opcode::BZR_BBS7:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.tl = co_await fetchOperand( state.pc++ );
      co_await read( state.ea );
      state.t = state.pc + ( int8_t )state.tl;
      if ( ( state.m1 & 0x80 ) != 0 )
      {
        state.pc = state.t;
      }
      break;
    case Opcode::BRK_BRK:
      if ( state.interrupt & CPUState::I_RESET )
      {
        co_await read( state.s );
        state.sl--;
        co_await read( state.s );
        state.sl--;
        co_await read( state.s );
        state.sl--;
        state.eal = co_await read( RESET_VECTOR );
        state.eah = co_await read( RESET_VECTOR + 1 );
      }
      else
      {
        //on state.interrupt PC should point to interrupted instruction
        if ( state.interrupt )
        {
          state.pc -= 1;
        }
        //on BRK PC should point past BRK argument
        else
        {
          state.pc += 1;
          //BRK is treated as NOP if mBreakOnBrk is true
          if ( mBreakOnBrk )
          {
            mReq.cpuBreakType = CpuBreakType::BRK_INSTRUCTION;
            break;
          }
          // "brk #$42" will be ignored
          if (state.ea == 0x42)
          {
              mReq.cpuBreakType = CpuBreakType::NONE;
              break;
          }
        }
        co_await write( state.s, state.pch );
        state.sl--;
        co_await write( state.s, state.pcl );
        state.sl--;
        co_await write( state.s, state.getP() );
        state.sl--;
        if ( state.interrupt & CPUState::I_NMI )
        {
          state.eal = co_await read( NMI_VECTOR );
          state.eah = co_await read( NMI_VECTOR + 1 );
        }
        else
        {
          state.eal = co_await read( IRQ_VECTOR );
          state.eah = co_await read( IRQ_VECTOR + 1 );
        }
        state.i.set();
      }
      state.d.clear();
      state.pc = state.ea;
      if ( mReq.cpuBreakType == CpuBreakType::STEP_OVER )
      {
        mReq.cpuBreakType = CpuBreakType::NONE;
        mStackBreakCondition = state.s;
      }
      break;
    case Opcode::RTI_RTI:
      ++state.pc;
      ++state.sl;
      state.setP( co_await read( state.s ) );
      ++state.sl;
      state.eal = co_await read( state.s );
      ++state.sl;
      state.eah = co_await read( state.s );
      if ( mStackBreakCondition < state.s )
      {
        mReq.cpuBreakType = mPostponedStepOut ? CpuBreakType::STEP_OUT : CpuBreakType::STEP_OVER;
        mPostponedStepOut = false;
        mStackBreakCondition = 0xffff;
      }
      co_await read( state.pc );
      state.pc = state.ea;
      break;
    case Opcode::RTS_RTS:
      co_await read( ++state.pc );
      ++state.sl;
      state.eal = co_await read( state.s );
      ++state.sl;
      state.eah = co_await read( state.s );
      if ( mStackBreakCondition < state.s )
      {
        mReq.cpuBreakType = mPostponedStepOut ? CpuBreakType::STEP_OUT : CpuBreakType::STEP_OVER;
        mPostponedStepOut = false;
        mStackBreakCondition = 0xffff;
      }
      co_await read( state.pc );
      ++state.ea;
      state.pc = state.ea;
      break;
    case Opcode::PHR_PHA:
      co_await write( state.s, state.a );
      state.sl--;
      break;
    case Opcode::PHR_PHP:
      co_await write( state.s, state.getP() );
      state.sl--;
      break;
    case Opcode::PHR_PHX:
      co_await write( state.s, state.x );
      state.sl--;
      break;
    case Opcode::PHR_PHY:
      co_await write( state.s, state.y );
      state.sl--;
      break;
    case Opcode::PLR_PLA:
      co_await read( state.pc );
      ++state.sl;
      state.setnz( state.a = co_await read( state.s ) );
      break;
    case Opcode::PLR_PLP:
      co_await read( state.pc );
      ++state.sl;
      state.setP( co_await read( state.s ) );
      break;
    case Opcode::PLR_PLX:
      co_await read( state.pc );
      ++state.sl;
      state.setnz( state.x = co_await read( state.s ) );
      break;
    case Opcode::PLR_PLY:
      co_await read( state.pc );
      ++state.sl;
      state.setnz( state.y = co_await read( state.s ) );
      break;
    case Opcode::UND_2_02:
    case Opcode::UND_2_22:
    case Opcode::UND_2_42:
    case Opcode::UND_2_62:
    case Opcode::UND_2_82:
    case Opcode::UND_2_C2:
    case Opcode::UND_2_E2:
      ++state.pc;
      break;
    case Opcode::UND_3_44:
      ++state.pc;
      co_await read( state.ea );
      break;
    case Opcode::UND_4_54:
    case Opcode::UND_4_d4:
    case Opcode::UND_4_f4:
      ++state.pc;
      co_await read( state.pc );
      state.tl = state.eal + state.x;
      co_await read( state.ea );
      break;
    case Opcode::UND_4_dc:
    case Opcode::UND_4_fc:
      ++state.pc;
      state.eah = co_await read( state.pc++ );
      co_await read( state.ea );
      break;
    case Opcode::UND_8_5c:
      //https://laughtonelectronics.com/Arcana/KimKlone/Kimklone_opcode_mapping.html
      //state.op - code 5C consumes 3 bytes and 8 cycles but conforms to no known address mode; it remains interesting but useless.
      //I tested the instruction "5C 1234h" ( stored little - endian as 5Ch 34h 12h ) as an example, and observed the following : 3 cycles fetching the instruction, 1 cycle reading FF34, then 4 cycles reading FFFF.
      ++state.pc;
      co_await read( state.pc++ );
      state.eah = 0xff;
      co_await read( state.ea );
      co_await read( 0xffff );
      co_await read( 0xffff );
      co_await read( 0xffff );
      co_await read( 0xffff );
      break;
    default:  //for UND_1_xx
      break;
    }

    trace2();

    do
    {
      co_await fetchOpcode( state.pc );
      mPreviousState = state;
      trace1();
      state.pc += 1;
    } while ( isHiccup() );
  }

}

void CPU::trace1()
{
  if ( mGlobalTrace )
  {
    buf[3] = hexTab[mPreviousState.pch >> 4];
    buf[4] = hexTab[mPreviousState.pch & 0x0f];
    buf[5] = hexTab[mPreviousState.pcl >> 4];
    buf[6] = hexTab[mPreviousState.pcl & 0x0f];

    buf[10] = hexTab[mPreviousState.a >> 4];
    buf[11] = hexTab[mPreviousState.a & 0x0f];
    buf[15] = hexTab[mPreviousState.x >> 4];
    buf[16] = hexTab[mPreviousState.x & 0x0f];
    buf[20] = hexTab[mPreviousState.y >> 4];
    buf[21] = hexTab[mPreviousState.y & 0x0f];

    buf[26] = hexTab[mPreviousState.sl >> 4];
    buf[27] = hexTab[mPreviousState.sl & 0x0f];

    mPreviousState.printP( (char*)&buf[31] );

    off = 38;
  }
}

void CPU::printStatus( std::span<uint8_t,3*14> text )
{
  static constexpr char prototype[3 * 14 + 1] =
    "A=ff X=ff Y=ff"
    "PC=ffff S=01ff"
    "P=NVDIZC      ";

  memcpy( text.data(), prototype, 3 * 14 );

  text[2] = hexTab[mState.a >> 4];
  text[3] = hexTab[mState.a & 0x0f];
  text[7] = hexTab[mState.x >> 4];
  text[8] = hexTab[mState.x & 0x0f];
  text[12] = hexTab[mState.y >> 4];
  text[13] = hexTab[mState.y & 0x0f];

  text[14 + 3] = hexTab[mState.pch >> 4];
  text[14 + 4] = hexTab[mState.pch & 0x0f];
  text[14 + 5] = hexTab[mState.pcl >> 4];
  text[14 + 6] = hexTab[mState.pcl & 0x0f];
  text[14 + 12] = hexTab[mState.sl >> 4];
  text[14 + 13] = hexTab[mState.sl & 0x0f];

  mState.printP( (char*)&text[28 + 2] );
}

void CPU::disassemblyFromPC( uint8_t const* ram, char* out, int columns, int rows )
{
  int pc = mState.pc;

  memset( out, ' ', rows * columns );
  out[rows * columns - 1] = 0;

  for ( size_t i = 0; i < rows; ++i )
  {
    auto base = (char*)&out[i * columns];
    base = da_sprintf(base, "%04x ", pc);
    (void)disasmOp( base, (Opcode)ram[pc] );
    disasmOpr( ram, (char*)base + 5, pc );
  }
}

void CPU::enableHistory( int columns, int rows )
{
  std::scoped_lock<std::mutex> l{ mHistoryMutex };
  mHistory.reset( new History( columns, rows, 0, std::vector<char>{} ) );
  mHistory->data.resize( columns * rows );
  mHistoryPresent.store( true );
  setGlobalTrace();
}

void CPU::disableHistory()
{
  mHistoryPresent.store( false );
  std::scoped_lock<std::mutex> l{ mHistoryMutex };
  mHistory.reset();
  setGlobalTrace();
}

void CPU::copyHistory( std::span<char> out )
{
  if ( mHistoryPresent.load() )
    mHistory->copy( out );
}

bool CPU::disasmOp( char * out, Opcode op, CPUState* state )
{
  bool defined = true;
  out[4] = ' '; /* Hack for history */

  switch ( op )
  {
  case Opcode::RZP_AND:
  case Opcode::RZX_AND:
  case Opcode::RIN_AND:
  case Opcode::RIX_AND:
  case Opcode::RIY_AND:
  case Opcode::RAB_AND:
  case Opcode::RAX_AND:
  case Opcode::RAY_AND:
  case Opcode::IMM_AND:
    *(uint32_t*)out = l4( "and " );
    break;
  case Opcode::RZP_BIT:
  case Opcode::RZX_BIT:
  case Opcode::RAB_BIT:
  case Opcode::RAX_BIT:
  case Opcode::IMM_BIT:
    *(uint32_t*)out = l4( "bit ");
    break;
  case Opcode::RZP_CMP:
  case Opcode::RZX_CMP:
  case Opcode::RIN_CMP:
  case Opcode::RIX_CMP:
  case Opcode::RIY_CMP:
  case Opcode::RAB_CMP:
  case Opcode::RAX_CMP:
  case Opcode::RAY_CMP:
  case Opcode::IMM_CMP:
    *(uint32_t*)out = l4( "cmp " );
    break;
  case Opcode::RZP_CPX:
  case Opcode::RAB_CPX:
  case Opcode::IMM_CPX:
    *(uint32_t*)out = l4( "cpx " );
    break;
  case Opcode::RZP_CPY:
  case Opcode::RAB_CPY:
  case Opcode::IMM_CPY:
    *(uint32_t*)out = l4( "cpy " );
    break;
  case Opcode::RZP_EOR:
  case Opcode::RZX_EOR:
  case Opcode::RIN_EOR:
  case Opcode::RIX_EOR:
  case Opcode::RIY_EOR:
  case Opcode::RAB_EOR:
  case Opcode::RAX_EOR:
  case Opcode::RAY_EOR:
  case Opcode::IMM_EOR:
    *(uint32_t*)out = l4( "eor " );
    break;
  case Opcode::RZP_LDA:
  case Opcode::RZX_LDA:
  case Opcode::RIN_LDA:
  case Opcode::RIX_LDA:
  case Opcode::RIY_LDA:
  case Opcode::RAB_LDA:
  case Opcode::RAX_LDA:
  case Opcode::RAY_LDA:
  case Opcode::IMM_LDA:
    *(uint32_t*)out = l4( "lda " );
    break;
  case Opcode::RZP_LDX:
  case Opcode::RZY_LDX:
  case Opcode::RAB_LDX:
  case Opcode::RAY_LDX:
  case Opcode::IMM_LDX:
    *(uint32_t*)out = l4( "ldx " );
    break;
  case Opcode::RZP_LDY:
  case Opcode::RZX_LDY:
  case Opcode::RAB_LDY:
  case Opcode::RAX_LDY:
  case Opcode::IMM_LDY:
    *(uint32_t*)out = l4( "ldy " );
    break;
  case Opcode::RZP_ORA:
  case Opcode::RZX_ORA:
  case Opcode::RIN_ORA:
  case Opcode::RIX_ORA:
  case Opcode::RIY_ORA:
  case Opcode::RAB_ORA:
  case Opcode::RAX_ORA:
  case Opcode::RAY_ORA:
  case Opcode::IMM_ORA:
    *(uint32_t*)out = l4( "ora " );
    break;
  case Opcode::RZP_ADC:
  case Opcode::RZX_ADC:
  case Opcode::RIN_ADC:
  case Opcode::RIX_ADC:
  case Opcode::RIY_ADC:
  case Opcode::RAB_ADC:
  case Opcode::RAX_ADC:
  case Opcode::RAY_ADC:
  case Opcode::IMM_ADC:
    *(uint32_t*)out = l4( "adc " );
    break;
  case Opcode::RZP_SBC:
  case Opcode::RZX_SBC:
  case Opcode::RIN_SBC:
  case Opcode::RIX_SBC:
  case Opcode::RIY_SBC:
  case Opcode::RAB_SBC:
  case Opcode::RAX_SBC:
  case Opcode::RAY_SBC:
  case Opcode::IMM_SBC:
    *(uint32_t*)out = l4( "sbc " );
    break;
  case Opcode::WZP_STA:
  case Opcode::WZX_STA:
  case Opcode::WIN_STA:
  case Opcode::WIX_STA:
  case Opcode::WIY_STA:
  case Opcode::WAB_STA:
  case Opcode::WAX_STA:
  case Opcode::WAY_STA:
    *(uint32_t*)out = l4( "sta " );
    break;
  case Opcode::WZP_STX:
  case Opcode::WZY_STX:
  case Opcode::WAB_STX:
    *(uint32_t*)out = l4( "stx " );
    break;
  case Opcode::WZP_STY:
  case Opcode::WZX_STY:
  case Opcode::WAB_STY:
    *(uint32_t*)out = l4( "sty " );
    break;
  case Opcode::WZP_STZ:
  case Opcode::WZX_STZ:
  case Opcode::WAB_STZ:
  case Opcode::WAX_STZ:
    *(uint32_t*)out = l4( "stz " );
    break;
  case Opcode::MZP_ASL:
  case Opcode::MZX_ASL:
  case Opcode::MAB_ASL:
  case Opcode::MAX_ASL:
    *(uint32_t*)out = l4( "asl " );
    break;
  case Opcode::IMP_ASL:
    *(uint32_t*)out = l4( "asl" );
    break;
  case Opcode::MZP_DEC:
  case Opcode::MZX_DEC:
  case Opcode::MAB_DEC:
  case Opcode::MAX_DEC:
    *(uint32_t*)out = l4( "dec " );
    break;
  case Opcode::IMP_DEC:
    *(uint32_t*)out = l4( "dec" );
    break;
  case Opcode::MZP_INC:
  case Opcode::MZX_INC:
  case Opcode::MAB_INC:
  case Opcode::MAX_INC:
  case Opcode::IMP_INC:
    *(uint32_t*)out = l4( "inc " );
    break;
  case Opcode::MZP_LSR:
  case Opcode::MZX_LSR:
  case Opcode::MAB_LSR:
  case Opcode::MAX_LSR:
  case Opcode::IMP_LSR:
    *(uint32_t*)out = l4( "lsr " );
    break;
  case Opcode::MZP_ROL:
  case Opcode::MZX_ROL:
  case Opcode::MAB_ROL:
  case Opcode::MAX_ROL:
  case Opcode::IMP_ROL:
    *(uint32_t*)out = l4( "rol " );
    break;
  case Opcode::MZP_ROR:
  case Opcode::MZX_ROR:
  case Opcode::MAB_ROR:
  case Opcode::MAX_ROR:
  case Opcode::IMP_ROR:
    *(uint32_t*)out = l4( "ror " );
    break;
  case Opcode::MZP_TRB:
  case Opcode::MAB_TRB:
    *(uint32_t*)out = l4( "trb " );
    break;
  case Opcode::MZP_TSB:
  case Opcode::MAB_TSB:
    *(uint32_t*)out = l4( "tsb " );
    break;
  case Opcode::MZP_RMB0:
    *(uint32_t*)out = l4( "rmb0" );
    break;
  case Opcode::MZP_RMB1:
    *(uint32_t*)out = l4( "rmb1" );
    break;
  case Opcode::MZP_RMB2:
    *(uint32_t*)out = l4( "rmb2" );
    break;
  case Opcode::MZP_RMB3:
    *(uint32_t*)out = l4( "rmb3" );
    break;
  case Opcode::MZP_RMB4:
    *(uint32_t*)out = l4( "rmb4" );
    break;
  case Opcode::MZP_RMB5:
    *(uint32_t*)out = l4( "rmb5" );
    break;
  case Opcode::MZP_RMB6:
    *(uint32_t*)out = l4( "rmb6" );
    break;
  case Opcode::MZP_RMB7:
    *(uint32_t*)out = l4( "rmb7" );
    break;
  case Opcode::MZP_SMB0:
    *(uint32_t*)out = l4( "smb0" );
    break;
  case Opcode::MZP_SMB1:
    *(uint32_t*)out = l4( "smb1" );
    break;
  case Opcode::MZP_SMB2:
    *(uint32_t*)out = l4( "smb2" );
    break;
  case Opcode::MZP_SMB3:
    *(uint32_t*)out = l4( "smb3" );
    break;
  case Opcode::MZP_SMB4:
    *(uint32_t*)out = l4( "smb4" );
    break;
  case Opcode::MZP_SMB5:
    *(uint32_t*)out = l4( "smb5" );
    break;
  case Opcode::MZP_SMB6:
    *(uint32_t*)out = l4( "smb6" );
    break;
  case Opcode::MZP_SMB7:
    *(uint32_t*)out = l4( "smb7" );
    break;
  case Opcode::JMA_JMP:
  case Opcode::JMX_JMP:
  case Opcode::JMI_JMP:
    *(uint32_t*)out = l4( "jmp " );
    break;
  case Opcode::JSA_JSR:
    *(uint32_t*)out = l4( "jsr " );
    break;
  case Opcode::IMP_CLC:
    *(uint32_t*)out = l4( "clc " );
    break;
  case Opcode::IMP_CLD:
    *(uint32_t*)out = l4( "cld " );
    break;
  case Opcode::IMP_CLI:
    *(uint32_t*)out = l4( "cli " );
    break;
  case Opcode::IMP_CLV:
    *(uint32_t*)out = l4( "clv " );
    break;
  case Opcode::IMP_DEX:
    *(uint32_t*)out = l4( "dex " );
    break;
  case Opcode::IMP_DEY:
    *(uint32_t*)out = l4( "dey " );
    break;
  case Opcode::IMP_INX:
    *(uint32_t*)out = l4( "inx " );
    break;
  case Opcode::IMP_INY:
    *(uint32_t*)out = l4( "iny " );
    break;
  case Opcode::IMP_SEC:
    *(uint32_t*)out = l4( "sec " );
    break;
  case Opcode::IMP_SED:
    *(uint32_t*)out = l4( "sed " );
    break;
  case Opcode::IMP_SEI:
    *(uint32_t*)out = l4( "sei " );
    break;
  case Opcode::IMP_TAX:
    *(uint32_t*)out = l4( "tax " );
    break;
  case Opcode::IMP_TAY:
    *(uint32_t*)out = l4( "tay " );
    break;
  case Opcode::IMP_TSX:
    *(uint32_t*)out = l4( "tsx " );
    break;
  case Opcode::IMP_TXA:
    *(uint32_t*)out = l4( "txa " );
    break;
  case Opcode::IMP_TXS:
    *(uint32_t*)out = l4( "txs " );
    break;
  case Opcode::IMP_TYA:
    *(uint32_t*)out = l4( "tya " );
    break;
  case Opcode::BRL_BCC:
    *(uint32_t*)out = l4( "bcc " );
    break;
  case Opcode::BRL_BCS:
    *(uint32_t*)out = l4( "bcs " );
    break;
  case Opcode::BRL_BEQ:
    *(uint32_t*)out = l4( "beq " );
    break;
  case Opcode::BRL_BMI:
    *(uint32_t*)out = l4( "bmi " );
    break;
  case Opcode::BRL_BNE:
    *(uint32_t*)out = l4( "bne " );
    break;
  case Opcode::BRL_BPL:
    *(uint32_t*)out = l4( "bpl " );
    break;
  case Opcode::BRL_BRA:
    *(uint32_t*)out = l4( "bra " );
    break;
  case Opcode::BRL_BVC:
    *(uint32_t*)out = l4( "bvc " );
    break;
  case Opcode::BRL_BVS:
    *(uint32_t*)out = l4( "bvs " );
    break;
  case Opcode::BZR_BBR0:
    *(uint32_t*)out = l4( "bbr0" );
    break;
  case Opcode::BZR_BBR1:
    *(uint32_t*)out = l4( "bbr1" );
    break;
  case Opcode::BZR_BBR2:
    *(uint32_t*)out = l4( "bbr2" );
    break;
  case Opcode::BZR_BBR3:
    *(uint32_t*)out = l4( "bbr3" );
    break;
  case Opcode::BZR_BBR4:
    *(uint32_t*)out = l4( "bbr4" );
    break;
  case Opcode::BZR_BBR5:
    *(uint32_t*)out = l4( "bbr5" );
    break;
  case Opcode::BZR_BBR6:
    *(uint32_t*)out = l4( "bbr6" );
    break;
  case Opcode::BZR_BBR7:
    *(uint32_t*)out = l4( "bbr7" );
    break;
  case Opcode::BZR_BBS0:
    *(uint32_t*)out = l4( "bbs0" );
    break;
  case Opcode::BZR_BBS1:
    *(uint32_t*)out = l4( "bbs1" );
    break;
  case Opcode::BZR_BBS2:
    *(uint32_t*)out = l4( "bbs2" );
    break;
  case Opcode::BZR_BBS3:
    *(uint32_t*)out = l4( "bbs3" );
    break;
  case Opcode::BZR_BBS4:
    *(uint32_t*)out = l4( "bbs4" );
    break;
  case Opcode::BZR_BBS5:
    *(uint32_t*)out = l4( "bbs5" );
    break;
  case Opcode::BZR_BBS6:
    *(uint32_t*)out = l4( "bbs6" );
    break;
  case Opcode::BZR_BBS7:
    *(uint32_t*)out = l4( "bbs7" );
    break;
  case Opcode::BRK_BRK:
    if ( state )
    {
      if ( ( state->interrupt & CPUState::I_RESET ) != 0 )
      {
        da_sprintf(out,"RESET");
        break;
      }
      else if ( ( state->interrupt & CPUState::I_NMI ) != 0 )
      {
        *(uint32_t*)out = l4( "NMI" );
        break;
      }
      else if ( ( state->interrupt & CPUState::I_IRQ ) != 0 )
      {
        *(uint32_t*)out = l4( "IRQ" );
        break;
      }
      else
      {
        *(uint32_t*)out = l4( "brk " );
        break;
      }
    }
    else
    {
      *(uint32_t*)out = l4( "brk " );
      break;
    }
  case Opcode::RTI_RTI:
    *(uint32_t*)out = l4( "rti " );
    break;
  case Opcode::RTS_RTS:
    *(uint32_t*)out = l4( "rts " );
    break;
  case Opcode::PHR_PHA:
    *(uint32_t*)out = l4( "pha " );
    break;
  case Opcode::PHR_PHP:
    *(uint32_t*)out = l4( "php " );
    break;
  case Opcode::PHR_PHX:
    *(uint32_t*)out = l4( "phx " );
    break;
  case Opcode::PHR_PHY:
    *(uint32_t*)out = l4( "phy " );
    break;
  case Opcode::PLR_PLA:
    *(uint32_t*)out = l4( "pla " );
    break;
  case Opcode::PLR_PLP:
    *(uint32_t*)out = l4( "plp " );
    break;
  case Opcode::PLR_PLX:
    *(uint32_t*)out = l4( "plx " );
    break;
  case Opcode::PLR_PLY:
    *(uint32_t*)out = l4( "ply " );
    break;
  case Opcode::IMP_NOP:
    *(uint32_t*)out = l4( "nop " );
    break;
  case Opcode::UND_1_03:
  case Opcode::UND_1_13:
  case Opcode::UND_1_23:
  case Opcode::UND_1_33:
  case Opcode::UND_1_43:
  case Opcode::UND_1_53:
  case Opcode::UND_1_63:
  case Opcode::UND_1_73:
  case Opcode::UND_1_83:
  case Opcode::UND_1_93:
  case Opcode::UND_1_a3:
  case Opcode::UND_1_b3:
  case Opcode::UND_1_c3:
  case Opcode::UND_1_d3:
  case Opcode::UND_1_e3:
  case Opcode::UND_1_f3:
  case Opcode::UND_1_0b:
  case Opcode::UND_1_1b:
  case Opcode::UND_1_2b:
  case Opcode::UND_1_3b:
  case Opcode::UND_1_4b:
  case Opcode::UND_1_5b:
  case Opcode::UND_1_6b:
  case Opcode::UND_1_7b:
  case Opcode::UND_1_8b:
  case Opcode::UND_1_9b:
  case Opcode::UND_1_ab:
  case Opcode::UND_1_bb:
  case Opcode::UND_1_cb:
  case Opcode::UND_1_db:
  case Opcode::UND_1_eb:
  case Opcode::UND_1_fb:
    *(uint32_t*)out = l4( "nop " );
    defined = false;
    break;
  case Opcode::UND_2_02:
  case Opcode::UND_2_22:
  case Opcode::UND_2_42:
  case Opcode::UND_2_62:
  case Opcode::UND_2_82:
  case Opcode::UND_2_C2:
  case Opcode::UND_2_E2:
  case Opcode::UND_3_44:
  case Opcode::UND_4_54:
  case Opcode::UND_4_d4:
  case Opcode::UND_4_f4:
  case Opcode::UND_4_dc:
  case Opcode::UND_4_fc:
  case Opcode::UND_8_5c:
    *(uint32_t*)out = l4( "nop " );
    defined = false;
    break;
  default:
    break;
  }

  return defined;
}

uint8_t CPU::disasmOpr( uint8_t const* ram, char* out, int & pc )
{
  Opcode op = (Opcode)ram[pc++];
  char *dst = out;
  uint8_t intialPC = pc;

  switch ( op )
  {
  case Opcode::UND_1_03:
  case Opcode::UND_1_13:
  case Opcode::UND_1_23:
  case Opcode::UND_1_33:
  case Opcode::UND_1_43:
  case Opcode::UND_1_53:
  case Opcode::UND_1_63:
  case Opcode::UND_1_73:
  case Opcode::UND_1_83:
  case Opcode::UND_1_93:
  case Opcode::UND_1_a3:
  case Opcode::UND_1_b3:
  case Opcode::UND_1_c3:
  case Opcode::UND_1_d3:
  case Opcode::UND_1_e3:
  case Opcode::UND_1_f3:
  case Opcode::UND_1_0b:
  case Opcode::UND_1_1b:
  case Opcode::UND_1_2b:
  case Opcode::UND_1_3b:
  case Opcode::UND_1_4b:
  case Opcode::UND_1_5b:
  case Opcode::UND_1_6b:
  case Opcode::UND_1_7b:
  case Opcode::UND_1_8b:
  case Opcode::UND_1_9b:
  case Opcode::UND_1_ab:
  case Opcode::UND_1_bb:
  case Opcode::UND_1_cb:
  case Opcode::UND_1_db:
  case Opcode::UND_1_eb:
  case Opcode::UND_1_fb:
  case Opcode::IMP_ASL:
  case Opcode::IMP_CLC:
  case Opcode::IMP_CLD:
  case Opcode::IMP_CLI:
  case Opcode::IMP_CLV:
  case Opcode::IMP_DEC:
  case Opcode::IMP_DEX:
  case Opcode::IMP_DEY:
  case Opcode::IMP_INC:
  case Opcode::IMP_INX:
  case Opcode::IMP_INY:
  case Opcode::IMP_LSR:
  case Opcode::IMP_NOP:
  case Opcode::IMP_ROL:
  case Opcode::IMP_ROR:
  case Opcode::IMP_SEC:
  case Opcode::IMP_SED:
  case Opcode::IMP_SEI:
  case Opcode::IMP_TAX:
  case Opcode::IMP_TAY:
  case Opcode::IMP_TSX:
  case Opcode::IMP_TXA:
  case Opcode::IMP_TXS:
  case Opcode::IMP_TYA:
  case Opcode::RTI_RTI:
  case Opcode::RTS_RTS:
  case Opcode::PHR_PHA:
  case Opcode::PHR_PHP:
  case Opcode::PHR_PHX:
  case Opcode::PHR_PHY:
  case Opcode::PLR_PLA:
  case Opcode::PLR_PLP:
  case Opcode::PLR_PLX:
  case Opcode::PLR_PLY:
    break;
  case Opcode::RZP_LDA:
  case Opcode::RZP_LDX:
  case Opcode::RZP_LDY:
  case Opcode::RZP_AND:
  case Opcode::RZP_BIT:
  case Opcode::RZP_CMP:
  case Opcode::RZP_CPX:
  case Opcode::RZP_CPY:
  case Opcode::RZP_EOR:
  case Opcode::RZP_ORA:
  case Opcode::RZP_ADC:
  case Opcode::RZP_SBC:
  case Opcode::MZP_ASL:
  case Opcode::MZP_DEC:
  case Opcode::MZP_INC:
  case Opcode::MZP_LSR:
  case Opcode::MZP_ROL:
  case Opcode::MZP_ROR:
  case Opcode::MZP_TRB:
  case Opcode::MZP_TSB:
  case Opcode::MZP_RMB0:
  case Opcode::MZP_RMB1:
  case Opcode::MZP_RMB2:
  case Opcode::MZP_RMB3:
  case Opcode::MZP_RMB4:
  case Opcode::MZP_RMB5:
  case Opcode::MZP_RMB6:
  case Opcode::MZP_RMB7:
  case Opcode::MZP_SMB0:
  case Opcode::MZP_SMB1:
  case Opcode::MZP_SMB2:
  case Opcode::MZP_SMB3:
  case Opcode::MZP_SMB4:
  case Opcode::MZP_SMB5:
  case Opcode::MZP_SMB6:
  case Opcode::MZP_SMB7:
  case Opcode::WZP_STA:
  case Opcode::WZP_STX:
  case Opcode::WZP_STY:
  case Opcode::WZP_STZ:
  case Opcode::UND_3_44:
  {
    int zp = ram[pc++];
    char const* txt = mTraceHelper->addressLabel( zp );
    (void)da_sprintf(dst, "%s\t;[%s]=$%02x", txt, txt, ram[zp]);
    break;
  }
  case Opcode::RZX_LDA:
  case Opcode::RZX_LDY:
  case Opcode::RZX_AND:
  case Opcode::RZX_BIT:
  case Opcode::RZX_CMP:
  case Opcode::RZX_EOR:
  case Opcode::RZX_ORA:
  case Opcode::RZX_ADC:
  case Opcode::RZX_SBC:
  case Opcode::MZX_ASL:
  case Opcode::MZX_DEC:
  case Opcode::MZX_INC:
  case Opcode::MZX_LSR:
  case Opcode::MZX_ROL:
  case Opcode::MZX_ROR:
  case Opcode::WZX_STA:
  case Opcode::WZX_STY:
  case Opcode::WZX_STZ:
  case Opcode::UND_4_54:
  case Opcode::UND_4_d4:
  case Opcode::UND_4_f4:
  {
    int zp = ram[pc++];
    int addr = (zp + mState.x) & 0xff;
    char const* txt = mTraceHelper->addressLabel( zp );
    (void)da_sprintf(dst, "%s,x\t;[$%02x]=$%02x", txt, addr, ram[addr]);
    break;
  }
  case Opcode::RZY_LDX:
  case Opcode::WZY_STX:
  {
    int zp = ram[pc++];
    int addr = (zp + mState.y) & 0xff;
    char const* txt = mTraceHelper->addressLabel( zp );
    (void)da_sprintf(dst, "%s,y\t;[$%02x]=$%02x", txt, addr, ram[addr]);
    break;
  }
  case Opcode::RIN_LDA:
  case Opcode::RIN_AND:
  case Opcode::RIN_CMP:
  case Opcode::RIN_EOR:
  case Opcode::RIN_ORA:
  case Opcode::RIN_ADC:
  case Opcode::RIN_SBC:
  case Opcode::WIN_STA:
  {
    int zp = ram[pc++];
    int addr = (ram[zp+1] << 8) | ram[zp];
    char const* txt = mTraceHelper->addressLabel( addr );
    if ( txt[0] == '$' )
    {
      int data = ram[addr];
      (void)da_sprintf(dst, "($%02x)\t;[$%04x]=$%02x", zp, addr, data);
    }
    else if ( pc == mState.pc + 2)
    {
      (void)da_sprintf(dst, "($%02x)\t;[$%04x]=$%02x", zp, addr, mState.m1);
    }
    else
    {
      (void)da_sprintf(dst, "($%02x)\t;[%s]", zp, txt);
    }
    break;
  }
  case Opcode::RIX_AND:
  case Opcode::RIX_CMP:
  case Opcode::RIX_EOR:
  case Opcode::RIX_LDA:
  case Opcode::RIX_ORA:
  case Opcode::RIX_ADC:
  case Opcode::RIX_SBC:
  case Opcode::WIX_STA:
  {
    int zp = ram[pc++];
    int addr = (ram[(zp + 1 + mState.x) & 0xff] << 8) | ram[(zp + mState.x) & 0xff];
    char const* txt = mTraceHelper->addressLabel( addr );
    if ( txt[0] == '$' )
    {
      int data = ram[addr];
      (void)da_sprintf(dst, "($%02x,x)\t;[$%04x]=$%02x", zp, addr, data);
    }
    else if ( pc == mState.pc + 2)
    {
      (void)da_sprintf(dst, "($%02x,x)\t;[$%04x]=$%02x", zp, addr, mState.m1);
    }
    else
    {
      (void)da_sprintf(dst, "($%02x,x)\t;[%s]", zp, txt);
    }
    break;
  }
  case Opcode::RIY_AND:
  case Opcode::RIY_CMP:
  case Opcode::RIY_EOR:
  case Opcode::RIY_LDA:
  case Opcode::RIY_ORA:
  case Opcode::RIY_ADC:
  case Opcode::RIY_SBC:
  case Opcode::WIY_STA:
  {
    int zp = ram[pc++];
    int addr = ((ram[(zp + 1) & 0xff] << 8) | ram[zp]) +  + mState.y;
    char const* txt = mTraceHelper->addressLabel( addr );
    if ( txt[0] == '$' )
    {
      int data = ram[addr];
      (void)da_sprintf(dst, "($%02x),y\t;[$%04x]=$%02x", zp, addr, data);
    }
    else if ( pc == mState.pc + 2)
    {
      (void)da_sprintf(dst, "($%02x),y\t;[$%04x]=$%02x", zp, addr, mState.m1);
    }
    else
    {
      (void)da_sprintf(dst, "($%02x),y\t;[%s]", zp, txt);
    }
    break;
  }
  case Opcode::RAB_AND:
  case Opcode::RAB_BIT:
  case Opcode::RAB_CMP:
  case Opcode::RAB_CPX:
  case Opcode::RAB_CPY:
  case Opcode::RAB_EOR:
  case Opcode::RAB_LDA:
  case Opcode::RAB_LDX:
  case Opcode::RAB_LDY:
  case Opcode::RAB_ORA:
  case Opcode::RAB_ADC:
  case Opcode::RAB_SBC:
  case Opcode::MAB_ASL:
  case Opcode::MAB_DEC:
  case Opcode::MAB_INC:
  case Opcode::MAB_LSR:
  case Opcode::MAB_ROL:
  case Opcode::MAB_ROR:
  case Opcode::MAB_TRB:
  case Opcode::MAB_TSB:
  case Opcode::WAB_STA:
  case Opcode::WAB_STX:
  case Opcode::WAB_STY:
  case Opcode::WAB_STZ:
  {
    int addr = (ram[pc + 1] << 8)|ram[pc];
    pc += 2;
    char const* txt = mTraceHelper->addressLabel( addr );
    if ( txt[0] == '$' )
    {
      int data = ram[addr];
      da_sprintf(dst, "%s\t;[$%04x]=$%02x", txt, addr, data);
    }
    else if ( pc == mState.pc + 3)
    {
      da_sprintf(dst, "%s\t;[$%04x]=$%02x", txt, addr, mState.m1);
    }
    else
    {
      da_sprintf(dst, "%s\t;[$%04x]", txt, addr);
    }
    break;
  }
  case Opcode::JMA_JMP:
  case Opcode::JSA_JSR:
  case Opcode::UND_4_dc:
  case Opcode::UND_4_fc:
  case Opcode::UND_8_5c:
  {
    int addr = (ram[pc + 1] << 8)|ram[pc];
    pc += 2;
    char const* txt = mTraceHelper->addressLabel( addr );
    da_sprintf(dst, "%s", txt);
    break;
  }

  case Opcode::RAX_AND:
  case Opcode::RAX_BIT:
  case Opcode::RAX_CMP:
  case Opcode::RAX_EOR:
  case Opcode::RAX_LDA:
  case Opcode::RAX_LDY:
  case Opcode::RAX_ORA:
  case Opcode::RAX_ADC:
  case Opcode::RAX_SBC:
  case Opcode::MAX_ASL:
  case Opcode::MAX_DEC:
  case Opcode::MAX_INC:
  case Opcode::MAX_LSR:
  case Opcode::MAX_ROL:
  case Opcode::MAX_ROR:
  case Opcode::WAX_STA:
  case Opcode::WAX_STZ:
  {
    int addr = (ram[pc + 1] << 8)|ram[pc];
    pc += 2;
    char const* txt = mTraceHelper->addressLabel( addr );
    if ( txt[0] == '$' )
    {
      int data = ram[addr + mState.x];
      da_sprintf(dst, "%s,x\t;[$%04x]=$%02x", txt, addr + mState.x, data);
    }
    else if ( pc == mState.pc + 3)
    {
      da_sprintf(dst, "%s,x\t;[$%04x]=$%02x", txt, addr + mState.x, mState.m1);
    }
    else
    {
      da_sprintf(dst, "%s,x\t;[$%04x]", txt, addr + mState.x);
    }
    break;
  }
  case Opcode::RAY_AND:
  case Opcode::RAY_CMP:
  case Opcode::RAY_EOR:
  case Opcode::RAY_LDA:
  case Opcode::RAY_LDX:
  case Opcode::RAY_ORA:
  case Opcode::RAY_ADC:
  case Opcode::RAY_SBC:
  case Opcode::WAY_STA:
  {
    int addr = (ram[pc + 1] << 8)|ram[pc];
    pc += 2;
    char const* txt = mTraceHelper->addressLabel( addr );
    addr += mState.y;
    if ( txt[0] == '$' )
    {
      int data = ram[addr];
      (void)da_sprintf(dst, "%s,y\t;[$%04x]=$%02x", txt, addr, data);
    }
    else if ( pc == mState.pc + 3)
    {
      (void)da_sprintf(dst, "%s,y\t;[$%04x]=$%02x", txt, addr, mState.m1);
    }
    else
    {
      (void)da_sprintf(dst, "%s,y\t;[$%04x]", txt, addr);
    }
    break;
  }
  case Opcode::JMX_JMP:
  {
    int addr = (ram[pc + 1] << 8)|ram[pc];
    pc += 2;
    char const* txt = mTraceHelper->addressLabel( addr );
    addr += mState.x;
    int jmp_dst = (ram[addr + 1] << 8)|ram[addr];
    (void)da_sprintf(dst, "(%s,x)\t;=>$%04x", txt, jmp_dst);
    break;
  }
  case Opcode::JMI_JMP:
  {
    int addr = (ram[pc+1] << 8) | ram[pc];
    pc += 2;
    int jmp_dst = (ram[addr+1] << 8) | ram[addr];
    char const* txt = mTraceHelper->addressLabel( addr );
    (void)da_sprintf(dst, "(%s)\t;=>$%04x", txt, jmp_dst);
    break;
  }
  case Opcode::IMM_AND:
  case Opcode::IMM_BIT:
  case Opcode::IMM_CMP:
  case Opcode::IMM_CPX:
  case Opcode::IMM_CPY:
  case Opcode::IMM_EOR:
  case Opcode::IMM_LDA:
  case Opcode::IMM_LDX:
  case Opcode::IMM_LDY:
  case Opcode::IMM_ORA:
  case Opcode::IMM_ADC:
  case Opcode::IMM_SBC:
  case Opcode::BRK_BRK:
  {
    int data = ram[pc++];
    (void)da_sprintf( dst, "#$%02x", data );
    break;
  }
  case Opcode::UND_2_02:
  case Opcode::UND_2_22:
  case Opcode::UND_2_42:
  case Opcode::UND_2_62:
  case Opcode::UND_2_82:
  case Opcode::UND_2_C2:
  case Opcode::UND_2_E2:
    break;
  case Opcode::BRL_BCC:
  case Opcode::BRL_BCS:
  case Opcode::BRL_BEQ:
  case Opcode::BRL_BMI:
  case Opcode::BRL_BNE:
  case Opcode::BRL_BPL:
  case Opcode::BRL_BVC:
  case Opcode::BRL_BVS:
  case Opcode::BRL_BRA:
  {
    int data = ram[pc++];
    int dest = pc + (int8_t)data;
    char const* txt = mTraceHelper->addressLabel( dest );
    (void)da_sprintf(dst, "%s", txt);
    break;
  }
  case Opcode::BZR_BBR0:
  case Opcode::BZR_BBR1:
  case Opcode::BZR_BBR2:
  case Opcode::BZR_BBR3:
  case Opcode::BZR_BBR4:
  case Opcode::BZR_BBR5:
  case Opcode::BZR_BBR6:
  case Opcode::BZR_BBR7:
  case Opcode::BZR_BBS0:
  case Opcode::BZR_BBS1:
  case Opcode::BZR_BBS2:
  case Opcode::BZR_BBS3:
  case Opcode::BZR_BBS4:
  case Opcode::BZR_BBS5:
  case Opcode::BZR_BBS6:
  case Opcode::BZR_BBS7:
  {
    int zp = ram[pc++];
    int data = ram[pc++];
    int dest = pc + (int8_t)data;
    char const* txt = mTraceHelper->addressLabel( dest );
    (void)da_sprintf(dst, "$%02x,%s\t;=[$%02x]=$%02x", zp,txt,zp, ram[zp]);
    break;
  }
  default:
    break;
  }

  return pc - intialPC;
}

void CPU::trace2()
{
  if ( !mGlobalTrace )
    return;

  disasmOp( buf.data() + off, mState.op, &mState );
  off += 5;

  auto comment = mTraceHelper->getTraceComment();

  switch ( mState.op )
  {
  case Opcode::UND_1_03:
  case Opcode::UND_1_13:
  case Opcode::UND_1_23:
  case Opcode::UND_1_33:
  case Opcode::UND_1_43:
  case Opcode::UND_1_53:
  case Opcode::UND_1_63:
  case Opcode::UND_1_73:
  case Opcode::UND_1_83:
  case Opcode::UND_1_93:
  case Opcode::UND_1_a3:
  case Opcode::UND_1_b3:
  case Opcode::UND_1_c3:
  case Opcode::UND_1_d3:
  case Opcode::UND_1_e3:
  case Opcode::UND_1_f3:
  case Opcode::UND_1_0b:
  case Opcode::UND_1_1b:
  case Opcode::UND_1_2b:
  case Opcode::UND_1_3b:
  case Opcode::UND_1_4b:
  case Opcode::UND_1_5b:
  case Opcode::UND_1_6b:
  case Opcode::UND_1_7b:
  case Opcode::UND_1_8b:
  case Opcode::UND_1_9b:
  case Opcode::UND_1_ab:
  case Opcode::UND_1_bb:
  case Opcode::UND_1_cb:
  case Opcode::UND_1_db:
  case Opcode::UND_1_eb:
  case Opcode::UND_1_fb:
    break;
  case Opcode::RZP_LDA:
  case Opcode::RZP_LDX:
  case Opcode::RZP_LDY:
  case Opcode::RZP_AND:
  case Opcode::RZP_BIT:
  case Opcode::RZP_CMP:
  case Opcode::RZP_CPX:
  case Opcode::RZP_CPY:
  case Opcode::RZP_EOR:
  case Opcode::RZP_ORA:
  case Opcode::RZP_ADC:
  case Opcode::RZP_SBC:
    off += sprintf( buf.data() + off, "$%02x\t;$%02x", mState.eal, mState.m1 );
    break;
  case Opcode::MZP_ASL:
  case Opcode::MZP_DEC:
  case Opcode::MZP_INC:
  case Opcode::MZP_LSR:
  case Opcode::MZP_ROL:
  case Opcode::MZP_ROR:
  case Opcode::MZP_TRB:
  case Opcode::MZP_TSB:
  case Opcode::MZP_RMB0:
  case Opcode::MZP_RMB1:
  case Opcode::MZP_RMB2:
  case Opcode::MZP_RMB3:
  case Opcode::MZP_RMB4:
  case Opcode::MZP_RMB5:
  case Opcode::MZP_RMB6:
  case Opcode::MZP_RMB7:
  case Opcode::MZP_SMB0:
  case Opcode::MZP_SMB1:
  case Opcode::MZP_SMB2:
  case Opcode::MZP_SMB3:
  case Opcode::MZP_SMB4:
  case Opcode::MZP_SMB5:
  case Opcode::MZP_SMB6:
  case Opcode::MZP_SMB7:
    off += sprintf( buf.data() + off, "$%02x\t;$%02x->$%02x", mState.eal, mState.m1, mState.m2 );
    break;
  case Opcode::WZP_STA:
  case Opcode::WZP_STX:
  case Opcode::WZP_STY:
  case Opcode::WZP_STZ:
  case Opcode::UND_3_44:
    off += sprintf( buf.data() + off, "$%02x", mState.eal );
    break;
  case Opcode::RZX_LDA:
  case Opcode::RZX_LDY:
  case Opcode::RZX_AND:
  case Opcode::RZX_BIT:
  case Opcode::RZX_CMP:
  case Opcode::RZX_EOR:
  case Opcode::RZX_ORA:
  case Opcode::RZX_ADC:
  case Opcode::RZX_SBC:
    off += sprintf( buf.data() + off, "$%02x,x\t;[$%04x]=$%02x", mState.eal, mState.t, mState.m1 );
    break;
  case Opcode::MZX_ASL:
  case Opcode::MZX_DEC:
  case Opcode::MZX_INC:
  case Opcode::MZX_LSR:
  case Opcode::MZX_ROL:
  case Opcode::MZX_ROR:
    off += sprintf( buf.data() + off, "$%02x,x\t;[$%04x]=$%02x->$%02x", mState.eal, mState.t, mState.m1, mState.m2 );
    break;
  case Opcode::WZX_STA:
  case Opcode::WZX_STY:
  case Opcode::WZX_STZ:
  case Opcode::UND_4_54:
  case Opcode::UND_4_d4:
  case Opcode::UND_4_f4:
    off += sprintf( buf.data() + off, "$%02x,x\t;[$%04x]", mState.eal, mState.t );
    break;
  case Opcode::RZY_LDX:
    off += sprintf( buf.data() + off, "$%02x,y\t;[$%04x]=$%02x", mState.eal, mState.t, mState.m1 );
    break;
  case Opcode::WZY_STX:
    off += sprintf( buf.data() + off, "$%02x,y\t;[$%04x]", mState.eal, mState.t );
    break;
  case Opcode::RIN_LDA:
  case Opcode::RIN_AND:
  case Opcode::RIN_CMP:
  case Opcode::RIN_EOR:
  case Opcode::RIN_ORA:
  case Opcode::RIN_ADC:
  case Opcode::RIN_SBC:
    if ( comment )
    {
      off = fmt::format_to( buf.data() + off, "({:02x})\t;[{}]={:02x}\t{}", mState.fa, mTraceHelper->addressLabel( mState.t ), mState.m1, *comment ) - buf.data();
    }
    else
    {
      off += sprintf( buf.data() + off, "($%02x)\t;[%s]=$%02x", mState.fa, mTraceHelper->addressLabel( mState.t ), mState.m1 );
    }
    break;
  case Opcode::WIN_STA:
    if ( comment )
    {
      off = fmt::format_to( buf.data() + off, "({:02x})\t;[{}]\t{}", mState.fa, mTraceHelper->addressLabel( mState.t ), *comment ) - buf.data();
    }
    else
    {
      off += sprintf( buf.data() + off, "($%02x)\t;[%s]", mState.fa, mTraceHelper->addressLabel( mState.t ) );
    }
    break;
  case Opcode::RIX_AND:
  case Opcode::RIX_CMP:
  case Opcode::RIX_EOR:
  case Opcode::RIX_LDA:
  case Opcode::RIX_ORA:
  case Opcode::RIX_ADC:
  case Opcode::RIX_SBC:
    if ( comment )
    {
      off = fmt::format_to( buf.data() + off, "({:02x},x)\t;[{}]={:02x}\t{}", mState.fa, mTraceHelper->addressLabel( mState.t ), mState.m1, *comment ) - buf.data();
    }
    else
    {
      off += sprintf( buf.data() + off, "($%02x,x)\t;[%s]=$%02x", mState.fa, mTraceHelper->addressLabel( mState.t ), mState.m1 );
    }
    break;
  case Opcode::WIX_STA:
    if ( comment )
    {
      off = fmt::format_to( buf.data() + off, "({:02x},x)\t;[{}]\t{}", mState.fa, mTraceHelper->addressLabel( mState.t ), *comment ) - buf.data();
    }
    else
    {
      off += sprintf( buf.data() + off, "($%02x,x)\t;[%s]", mState.fa, mTraceHelper->addressLabel( mState.t ) );
    }
    break;
  case Opcode::RIY_AND:
  case Opcode::RIY_CMP:
  case Opcode::RIY_EOR:
  case Opcode::RIY_LDA:
  case Opcode::RIY_ORA:
  case Opcode::RIY_ADC:
  case Opcode::RIY_SBC:
    if ( comment )
    {
      off = fmt::format_to( buf.data() + off, "({:02x}),y\t;[{}]={:02x}\t{}", mState.fa, mTraceHelper->addressLabel( mState.ea ), mState.m1, *comment ) - buf.data();
    }
    else
    {
      off += sprintf( buf.data() + off, "($%02x),y\t;[%s]=$%02x", mState.fa, mTraceHelper->addressLabel( mState.ea ), mState.m1 );
    }
    break;
  case Opcode::WIY_STA:
    if ( comment )
    {
      off = fmt::format_to( buf.data() + off, "({:02x}),y\t;[{}]\t{}", mState.fa, mTraceHelper->addressLabel( mState.ea ), *comment ) - buf.data();
    }
    else
    {
      off += sprintf( buf.data() + off, "($%02x),y\t;[%s]", mState.fa, mTraceHelper->addressLabel( mState.ea ) );
    }
    break;
  case Opcode::RAB_AND:
  case Opcode::RAB_BIT:
  case Opcode::RAB_CMP:
  case Opcode::RAB_CPX:
  case Opcode::RAB_CPY:
  case Opcode::RAB_EOR:
  case Opcode::RAB_LDA:
  case Opcode::RAB_LDX:
  case Opcode::RAB_LDY:
  case Opcode::RAB_ORA:
  case Opcode::RAB_ADC:
  case Opcode::RAB_SBC:
    if ( comment )
    {
      off = fmt::format_to( buf.data() + off, "{}\t;={:02x}\t{}", mTraceHelper->addressLabel( mState.ea ), mState.m1, *comment ) - buf.data();
    }
    else
    {
      off += sprintf( buf.data() + off, "%s\t;=$%02x", mTraceHelper->addressLabel( mState.ea ), mState.m1 );
    }
    break;
  case Opcode::MAB_ASL:
  case Opcode::MAB_DEC:
  case Opcode::MAB_INC:
  case Opcode::MAB_LSR:
  case Opcode::MAB_ROL:
  case Opcode::MAB_ROR:
  case Opcode::MAB_TRB:
  case Opcode::MAB_TSB:
    if ( comment )
    {
      off = fmt::format_to( buf.data() + off, "{}\t;={:02x}->{:02x}\t{}", mTraceHelper->addressLabel( mState.ea ), mState.m1, mState.m2, *comment ) - buf.data();
    }
    else
    {
      off += sprintf( buf.data() + off, "%s\t;=$%02x->$%02x", mTraceHelper->addressLabel( mState.ea ), mState.m1, mState.m2 );
    }
    break;
  case Opcode::WAB_STA:
  case Opcode::WAB_STX:
  case Opcode::WAB_STY:
  case Opcode::WAB_STZ:
    if ( comment )
    {
      off = fmt::format_to( buf.data() + off, "{}\t;{}", mTraceHelper->addressLabel( mState.ea ), *comment ) - buf.data();
    }
    else
    {
      off += sprintf( buf.data() + off, "%s", mTraceHelper->addressLabel( mState.ea ) );
    }
    break;
  case Opcode::JMA_JMP:
  case Opcode::JSA_JSR:
  case Opcode::UND_4_dc:
  case Opcode::UND_4_fc:
  case Opcode::UND_8_5c:
    off += sprintf( buf.data() + off, "%s", mTraceHelper->addressLabel( mState.ea ) );
    break;
  case Opcode::RAX_AND:
  case Opcode::RAX_BIT:
  case Opcode::RAX_CMP:
  case Opcode::RAX_EOR:
  case Opcode::RAX_LDA:
  case Opcode::RAX_LDY:
  case Opcode::RAX_ORA:
  case Opcode::RAX_ADC:
  case Opcode::RAX_SBC:
    if ( comment )
    {
      off = fmt::format_to( buf.data() + off, "{:04x},x\t;[{}]={:02x}\t{}", mState.ea, mTraceHelper->addressLabel( mState.fa ), mState.m1, *comment ) - buf.data();
    }
    else
    {
      off += sprintf( buf.data() + off, "$%04x,x\t;[%s]=$%02x", mState.ea, mTraceHelper->addressLabel( mState.fa ), mState.m1 );
    }
    break;
  case Opcode::MAX_ASL:
  case Opcode::MAX_DEC:
  case Opcode::MAX_INC:
  case Opcode::MAX_LSR:
  case Opcode::MAX_ROL:
  case Opcode::MAX_ROR:
    if ( comment )
    {
      off = fmt::format_to( buf.data() + off, "{:04x},x\t;[{}]={:02x}->{:02x}\t{}", mState.ea, mTraceHelper->addressLabel( mState.fa ), mState.m1, mState.m2, *comment ) - buf.data();
    }
    else
    {
      off += sprintf( buf.data() + off, "$%04x,x\t;[%s]=$%02x->$%02x", mState.ea, mTraceHelper->addressLabel( mState.fa ), mState.m1, mState.m2 );
    }
    break;
  case Opcode::WAX_STA:
  case Opcode::WAX_STZ:
    if ( comment )
    {
      off = fmt::format_to( buf.data() + off, "{:04x},x\t;[{}]\t{}", mState.ea, mTraceHelper->addressLabel( mState.fa ), *comment ) - buf.data();
    }
    else
    {
      off += sprintf( buf.data() + off, "$%04x,x\t;[%s]", mState.ea, mTraceHelper->addressLabel( mState.fa ) );
    }
    break;
  case Opcode::RAY_AND:
  case Opcode::RAY_CMP:
  case Opcode::RAY_EOR:
  case Opcode::RAY_LDA:
  case Opcode::RAY_LDX:
  case Opcode::RAY_ORA:
  case Opcode::RAY_ADC:
  case Opcode::RAY_SBC:
    if ( comment )
    {
      off = fmt::format_to( buf.data() + off, "{:04x},y\t;[{}]={:02x}\t{}", mState.ea, mTraceHelper->addressLabel( mState.fa ), mState.m1, *comment ) - buf.data();
    }
    else
    {
      off += sprintf( buf.data() + off, "$%04x,y\t;[%s]=$%02x", mState.ea, mTraceHelper->addressLabel( mState.fa ), mState.m1 );
    }
    break;
  case Opcode::WAY_STA:
    if ( comment )
    {
      off = fmt::format_to( buf.data() + off, "{:04x},y\t;[{}]\t{}", mState.ea, mTraceHelper->addressLabel( mState.fa ), *comment ) - buf.data();
    }
    else
    {
      off += sprintf( buf.data() + off, "$%04x,y\t;[%s]", mState.ea, mTraceHelper->addressLabel( mState.fa ) );
    }
    break;
  case Opcode::JMX_JMP:
    off += sprintf( buf.data() + off, "($%04x,x)\t;[%s]", mState.fa, mTraceHelper->addressLabel( mState.ea ) );
    break;
  case Opcode::JMI_JMP:
    off += sprintf( buf.data() + off, "($%04x)\t;[%s]", mState.fa, mTraceHelper->addressLabel( mState.t ) );
    break;
  case Opcode::IMP_ASL:
  case Opcode::IMP_CLC:
  case Opcode::IMP_CLD:
  case Opcode::IMP_CLI:
  case Opcode::IMP_CLV:
  case Opcode::IMP_DEC:
  case Opcode::IMP_DEX:
  case Opcode::IMP_DEY:
  case Opcode::IMP_INC:
  case Opcode::IMP_INX:
  case Opcode::IMP_INY:
  case Opcode::IMP_LSR:
  case Opcode::IMP_NOP:
  case Opcode::IMP_ROL:
  case Opcode::IMP_ROR:
  case Opcode::IMP_SEC:
  case Opcode::IMP_SED:
  case Opcode::IMP_SEI:
  case Opcode::IMP_TAX:
  case Opcode::IMP_TAY:
  case Opcode::IMP_TSX:
  case Opcode::IMP_TXA:
  case Opcode::IMP_TXS:
  case Opcode::IMP_TYA:
  case Opcode::RTI_RTI:
  case Opcode::RTS_RTS:
  case Opcode::PHR_PHA:
  case Opcode::PHR_PHP:
  case Opcode::PHR_PHX:
  case Opcode::PHR_PHY:
  case Opcode::PLR_PLA:
  case Opcode::PLR_PLP:
  case Opcode::PLR_PLX:
  case Opcode::PLR_PLY:
    break;
  case Opcode::IMM_AND:
  case Opcode::IMM_BIT:
  case Opcode::IMM_CMP:
  case Opcode::IMM_CPX:
  case Opcode::IMM_CPY:
  case Opcode::IMM_EOR:
  case Opcode::IMM_LDA:
  case Opcode::IMM_LDX:
  case Opcode::IMM_LDY:
  case Opcode::IMM_ORA:
  case Opcode::IMM_ADC:
  case Opcode::IMM_SBC:
  case Opcode::UND_2_02:
  case Opcode::UND_2_22:
  case Opcode::UND_2_42:
  case Opcode::UND_2_62:
  case Opcode::UND_2_82:
  case Opcode::UND_2_C2:
  case Opcode::UND_2_E2:
  case Opcode::BRK_BRK:
    off += sprintf( buf.data() + off, "#$%02x", mState.eal );
    break;
  case Opcode::BRL_BCC:
  case Opcode::BRL_BCS:
  case Opcode::BRL_BEQ:
  case Opcode::BRL_BMI:
  case Opcode::BRL_BNE:
  case Opcode::BRL_BPL:
  case Opcode::BRL_BVC:
  case Opcode::BRL_BVS:
  case Opcode::BRL_BRA:
    off += sprintf( buf.data() + off, "$%04x", mState.t );
    break;
  case Opcode::BZR_BBR0:
  case Opcode::BZR_BBR1:
  case Opcode::BZR_BBR2:
  case Opcode::BZR_BBR3:
  case Opcode::BZR_BBR4:
  case Opcode::BZR_BBR5:
  case Opcode::BZR_BBR6:
  case Opcode::BZR_BBR7:
  case Opcode::BZR_BBS0:
  case Opcode::BZR_BBS1:
  case Opcode::BZR_BBS2:
  case Opcode::BZR_BBS3:
  case Opcode::BZR_BBS4:
  case Opcode::BZR_BBS5:
  case Opcode::BZR_BBS6:
  case Opcode::BZR_BBS7:
    off += sprintf( buf.data() + off, "$%02x,$%04x\t;$%02x", mState.eal, mState.t, mState.m1 );
    break;
  }

  if ( mFtrace.good() && ( mTrace || mTraceNextCount ) )
  {
    mFtrace.write( buf.data(), off );
    mFtrace.put( '\n' );
  }

  if ( mTraceNextCount )
  {
    if ( --mTraceNextCount == 0 )
    {
      toggleTrace( false );
    }
  }

  std::scoped_lock<std::mutex> l{ mHistoryMutex };
  if ( mHistoryPresent.load() )
  {
    auto row = mHistory->nextRow();
    auto it = std::copy_n( buf.cbegin(), (std::min)( row.size(), (size_t)off ), row.begin() );
    std::fill( it, row.end(), ' ' );
  }
}

void CPU::enableTrace()
{
  mTraceHelper->enable( true );
  mTrace = true;
  setGlobalTrace();
}

void CPU::disableTrace()
{
  mTraceHelper->enable( false );
  mTrace = false;
  setGlobalTrace();
}

void CPU::toggleTrace( bool on )
{
  mTraceHelper->enable( on );
  mTraceNextCount = on ? 1 : 0;
  setGlobalTrace();
}

void CPU::traceNextCount( int count )
{
  mTraceHelper->enable( count > 0 );
  mTraceNextCount = count;
  setGlobalTrace();
}


void CPU::setGlobalTrace()
{
  if ( mTrace || mTraceNextCount || mHistoryPresent.load() )
  {
    if ( !mGlobalTrace )
    {
      mGlobalTrace = true;
      trace1();
    }
  }
  else
  {
    mGlobalTrace = false;
  }
}


std::span<char> CPU::History::nextRow()
{
  std::span<char> result = std::span<char>{ data.data() + cursor * columns, (size_t)columns };
  cursor = ( cursor + 1 ) % rows;

  return result;
}

void CPU::History::copy( std::span<char> out )
{
  int cur = cursor;
  size_t i = 0;

  do
  {
    std::copy_n( data.data() + cur * columns, columns, out.data() + i * columns );
    cur = ( cur + 1 ) % rows;
  } while ( ++i < rows );
}

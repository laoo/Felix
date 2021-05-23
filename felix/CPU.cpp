#include "pch.hpp"
#include "CPU.hpp"
#include "Opcodes.hpp"
#include "Felix.hpp"

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

CPUState & CPU::state()
{
  return mState;
}

CPU::CPU( Felix & felix, bool trace ) : felix{ felix }, mState{}, operand{}, mEx{ execute() }, mReq{}, mRes{ mState }, mTrace{ trace }
{
  if ( mTrace )
  {
    mFtrace = std::ofstream{ "trace.log" };
  }
}

CPU::Request const& CPU::advance()
{
  mEx.coro();
  return mReq;
}

void CPU::respond( uint8_t value )
{
  mRes.value = value;
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

void CPU::respond( uint64_t tick, uint8_t value )
{
  mRes.tick = tick;
  mRes.value = value;
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
  trace1();

  auto & state = mState;

  for ( ;; )
  {
    state.ea = 0;
    state.t = 0;

    if ( state.interrupt && ( !get<bitI>() || ( state.interrupt & ~CPUState::I_IRQ ) != 0 ) )
    {
      state.op = Opcode::BRK_BRK;
    }

    state.eal = co_await fetchOperand( state.pc );

    switch ( state.op )
    {
    case Opcode::RZP_AND:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      setnz( state.a &= state.m1 );
      break;
    case Opcode::RZP_BIT:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      bit( state.m1 );
      break;
    case Opcode::RZP_CMP:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      cmp( state.m1 );
      break;
    case Opcode::RZP_CPX:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      cpx( state.m1 );
      break;
    case Opcode::RZP_CPY:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      cpy( state.m1 );
      break;
    case Opcode::RZP_EOR:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      setnz( state.a ^= state.m1 );
      break;
    case Opcode::RZP_LDA:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      setnz( state.a = state.m1 );
      break;
    case Opcode::RZP_LDX:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      setnz( state.x = state.m1 );
      break;
    case Opcode::RZP_LDY:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      setnz( state.y = state.m1 );
      break;
    case Opcode::RZP_ORA:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      setnz( state.a |= state.m1 );
      break;
    case Opcode::RZP_ADC:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      adc( state.m1 );
      if ( get<bitD>() )
      {
        co_await read( state.ea );
      }
      break;
    case Opcode::RZP_SBC:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      sbc( state.m1 );
      if ( get<bitD>() )
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
      state.m2 = asl( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_DEC:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = dec( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_INC:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = inc( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_LSR:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = lsr( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_ROL:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = rol( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_ROR:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = ror( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_TRB:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      setz( state.m1 & state.a );
      state.m2 = state.m1 & ~state.a;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_TSB:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      setz( state.m1 & state.a );
      state.m2 = state.m1 | state.a;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_RMB0:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = state.m1 & ~0x01;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_RMB1:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = state.m1 & ~0x02;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_RMB2:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = state.m1 & ~0x04;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_RMB3:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = state.m1 & ~0x08;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_RMB4:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = state.m1 & ~0x10;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_RMB5:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = state.m1 & ~0x20;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_RMB6:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = state.m1 & ~0x40;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_RMB7:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = state.m1 & ~0x80;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_SMB0:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = state.m1 | 0x01;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_SMB1:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = state.m1 | 0x02;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_SMB2:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = state.m1 | 0x04;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_SMB3:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = state.m1 | 0x08;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_SMB4:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = state.m1 | 0x10;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_SMB5:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = state.m1 | 0x20;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_SMB6:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = state.m1 | 0x40;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MZP_SMB7:
      ++state.pc;
      state.m1 = co_await read( state.ea );
      state.m2 = state.m1 | 0x80;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::RZX_AND:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      setnz( state.a &= state.m1 );
      break;
    case Opcode::RZX_BIT:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      bit( state.m1 );
      break;
    case Opcode::RZX_CMP:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      cmp( state.m1 );
      break;
    case Opcode::RZX_EOR:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      setnz( state.a ^= state.m1 );
      break;
    case Opcode::RZX_LDA:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      setnz( state.a = state.m1 );
      break;
    case Opcode::RZX_LDY:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      setnz( state.y = state.m1 );
      break;
    case Opcode::RZX_ORA:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      setnz( state.a |= state.m1 );
      break;
    case Opcode::RZX_ADC:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      adc( state.m1 );
      if ( get<bitD>() )
      {
        co_await read( state.t );
      }
      break;
    case Opcode::RZX_SBC:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      sbc( state.m1 );
      if ( get<bitD>() )
      {
        co_await read( state.t );
      }
      break;
    case Opcode::RZY_LDX:
      co_await read( ++state.pc );
      state.tl = state.eal + state.y;
      state.m1 = co_await read( state.t );
      setnz( state.x = state.m1 );
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
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      state.m2 = asl( state.m1 );
      co_await write( state.t, state.m2 );
      break;
    case Opcode::MZX_DEC:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      state.m2 = dec( state.m1 );
      co_await write( state.t, state.m2 );
      break;
    case Opcode::MZX_INC:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      state.m2 = inc( state.m1 );
      co_await write( state.t, state.m2 );
      break;
    case Opcode::MZX_LSR:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      state.m2 = lsr( state.m1 );
      co_await write( state.t, state.m2 );
      break;
    case Opcode::MZX_ROL:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      state.m2 = rol( state.m1 );
      co_await write( state.t, state.m2 );
      break;
    case Opcode::MZX_ROR:
      co_await read( ++state.pc );
      state.tl = state.eal + state.x;
      state.m1 = co_await read( state.t );
      state.m2 = ror( state.m1 );
      co_await write( state.t, state.m2 );
      break;
    case Opcode::RIN_AND:
      ++state.pc;
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      setnz( state.a &= state.m1 );
      break;
    case Opcode::RIN_CMP:
      ++state.pc;
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      cmp( state.m1 );
      break;
    case Opcode::RIN_EOR:
      ++state.pc;
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      setnz( state.a ^= state.m1 );
      break;
    case Opcode::RIN_LDA:
      ++state.pc;
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      setnz( state.a = state.m1 );
      break;
    case Opcode::RIN_ORA:
      ++state.pc;
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      setnz( state.a |= state.m1 );
      break;
    case Opcode::RIN_ADC:
      ++state.pc;
      state.fa = state.ea;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      adc( state.m1 );
      if ( get<bitD>() )
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
      sbc( state.m1 );
      if ( get<bitD>() )
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
      setnz( state.a &= state.m1 );
      break;
    case Opcode::RIX_CMP:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.eal += state.x;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      cmp( state.m1 );
      break;
    case Opcode::RIX_EOR:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.eal += state.x;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      setnz( state.a ^= state.m1 );
      break;
    case Opcode::RIX_LDA:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.eal += state.x;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      setnz( state.a = state.m1 );
      break;
    case Opcode::RIX_ORA:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.eal += state.x;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      setnz( state.a |= state.m1 );
      break;
    case Opcode::RIX_ADC:
      co_await read( ++state.pc );
      state.fa = state.ea;
      state.eal += state.x;
      state.tl = co_await read( state.ea++ );
      state.th = co_await read( state.ea );
      state.m1 = co_await read( state.t );
      adc( state.m1 );
      if ( get<bitD>() )
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
      sbc( state.m1 );
      if ( get<bitD>() )
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
      setnz( state.a &= state.m1 );
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
      cmp( state.m1 );
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
      setnz( state.a ^= state.m1 );
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
      setnz( state.a = state.m1 );
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
      setnz( state.a |= state.m1 );
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
      adc( state.m1 );
      if ( get<bitD>() )
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
      sbc( state.m1 );
      if ( get<bitD>() )
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
      setnz( state.a &= state.m1 );
      break;
    case Opcode::RAB_BIT:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      bit( state.m1 );
      break;
    case Opcode::RAB_CMP:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      cmp( state.m1 );
      break;
    case Opcode::RAB_CPX:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      cpx( state.m1 );
      break;
    case Opcode::RAB_CPY:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      cpy( state.m1 );
      break;
    case Opcode::RAB_EOR:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      setnz( state.a ^= state.m1 );
      break;
    case Opcode::RAB_LDA:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      setnz( state.a = state.m1 );
      break;
    case Opcode::RAB_LDX:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      setnz( state.x = state.m1 );
      break;
    case Opcode::RAB_LDY:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      setnz( state.y = state.m1 );
      break;
    case Opcode::RAB_ORA:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      setnz( state.a |= state.m1 );
      break;
    case Opcode::RAB_ADC:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      adc( state.m1 );
      if ( get<bitD>() )
      {
        co_await read( state.ea );
      }
      break;
    case Opcode::RAB_SBC:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );      
      sbc( state.m1 );
      if ( get<bitD>() )
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
      state.m2 = asl( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAB_DEC:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      state.m2 = dec( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAB_INC:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      state.m2 = inc( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAB_LSR:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      state.m2 = lsr( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAB_ROL:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      state.m2 = rol( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAB_ROR:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      state.m2 = ror( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAB_TRB:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      setz( state.m1 & state.a );
      state.m2 = state.m1 & ~state.a;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAB_TSB:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.m1 = co_await read( state.ea );
      setz( state.m1 & state.a );
      state.m2 = state.m1 | state.a;
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::RAX_AND:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.t += state.x;
      if ( state.th != state.eah )
      {
        state.eal += state.x;
        co_await read( state.ea );
      }
      state.m1 = co_await read( state.t );
      setnz( state.a &= state.m1 );
      break;
    case Opcode::RAX_BIT:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.t += state.x;
      if ( state.th != state.eah )
      {
        state.eal += state.x;
        co_await read( state.ea );
      }
      state.m1 = co_await read( state.t );
      bit( state.m1 );
      break;
    case Opcode::RAX_CMP:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.t += state.x;
      if ( state.th != state.eah )
      {
        state.eal += state.x;
        co_await read( state.ea );
      }
      state.m1 = co_await read( state.t );
      cmp( state.m1 );
      break;
    case Opcode::RAX_EOR:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.t += state.x;
      if ( state.th != state.eah )
      {
        state.eal += state.x;
        co_await read( state.ea );
      }
      state.m1 = co_await read( state.t );
      setnz( state.a ^= state.m1 );
      break;
    case Opcode::RAX_LDA:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.t += state.x;
      if ( state.th != state.eah )
      {
        state.eal += state.x;
        co_await read( state.ea );
      }
      state.m1 = co_await read( state.t );
      setnz( state.a = state.m1 );
      break;
    case Opcode::RAX_LDY:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.t += state.x;
      if ( state.th != state.eah )
      {
        state.eal += state.x;
        co_await read( state.ea );
      }
      state.m1 = co_await read( state.t );
      setnz( state.y = state.m1 );
      break;
    case Opcode::RAX_ORA:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.t += state.x;
      if ( state.th != state.eah )
      {
        state.eal += state.x;
        co_await read( state.ea );
      }
      state.m1 = co_await read( state.t );
      setnz( state.a |= state.m1 );
      break;
    case Opcode::RAX_ADC:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.t += state.x;
      if ( state.th != state.eah )
      {
        state.eal += state.x;
        co_await read( state.ea );
      }
      state.m1 = co_await read( state.t );
      adc( state.m1 );
      if ( get<bitD>() )
      {
        co_await read( state.t );
      }
      break;
    case Opcode::RAX_SBC:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.t += state.x;
      if ( state.th != state.eah )
      {
        state.eal += state.x;
        co_await read( state.ea );
      }
      state.m1 = co_await read( state.t );
      sbc( state.m1 );
      if ( get<bitD>() )
      {
        co_await read( state.t );
      }
      break;
    case Opcode::RAY_AND:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.t += state.y;
      if ( state.th != state.eah )
      {
        state.eal += state.x;
        co_await read( state.ea );
      }
      state.m1 = co_await read( state.t );
      setnz( state.a &= state.m1 );
      break;
    case Opcode::RAY_CMP:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.t += state.y;
      if ( state.th != state.eah )
      {
        state.eal += state.x;
        co_await read( state.ea );
      }
      state.m1 = co_await read( state.t );
      cmp( state.m1 );
      break;
    case Opcode::RAY_EOR:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.t += state.y;
      if ( state.th != state.eah )
      {
        state.eal += state.x;
        co_await read( state.ea );
      }
      state.m1 = co_await read( state.t );
      setnz( state.a ^= state.m1 );
      break;
    case Opcode::RAY_LDA:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.t += state.y;
      if ( state.th != state.eah )
      {
        state.eal += state.x;
        co_await read( state.ea );
      }
      state.m1 = co_await read( state.t );
      setnz( state.a = state.m1 );
      break;
    case Opcode::RAY_LDX:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.t += state.y;
      if ( state.th != state.eah )
      {
        state.eal += state.x;
        co_await read( state.ea );
      }
      state.m1 = co_await read( state.t );
      setnz( state.x = state.m1 );
      break;
    case Opcode::RAY_ORA:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.t += state.y;
      if ( state.th != state.eah )
      {
        state.eal += state.x;
        co_await read( state.ea );
      }
      state.m1 = co_await read( state.t );
      setnz( state.a |= state.m1 );
      break;
    case Opcode::RAY_ADC:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.t += state.y;
      if ( state.th != state.eah )
      {
        state.eal += state.x;
        co_await read( state.ea );
      }
      state.m1 = co_await read( state.t );
      adc( state.m1 );
      if ( get<bitD>() )
      {
        co_await read( state.t );
      }
      break;
    case Opcode::RAY_SBC:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.t += state.y;
      if ( state.th != state.eah )
      {
        state.eal += state.x;
        co_await read( state.ea );
      }
      state.m1 = co_await read( state.t );
      sbc( state.m1 );
      if ( get<bitD>() )
      {
        co_await read( state.t );
      }
      break;
    case Opcode::WAX_STA:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.eal += state.x;
      state.t += state.x;
      co_await read( state.ea );
      co_await write( state.t, state.a );
      break;
    case Opcode::WAX_STZ:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.eal += state.x;
      state.t += state.x;
      co_await read( state.ea );
      co_await write( state.t, 0x00 );
      break;
    case Opcode::WAY_STA:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.eal += state.y;
      state.t += state.y;
      co_await read( state.ea );
      co_await write( state.t, state.a );
      break;
    case Opcode::MAX_ASL:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.eal += state.x;
      state.t += state.x;
      co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.m2 = asl( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAX_DEC:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.eal += state.x;
      state.t += state.x;
      co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.m2 = dec( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAX_INC:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.eal += state.x;
      state.t += state.x;
      co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.m2 = inc( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAX_LSR:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.eal += state.x;
      state.t += state.x;
      co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.m2 = lsr( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAX_ROL:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.eal += state.x;
      state.t += state.x;
      co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.m2 = rol( state.m1 );
      co_await write( state.ea, state.m2 );
      break;
    case Opcode::MAX_ROR:
      ++state.pc;
      state.eah = co_await fetchOperand( state.pc++ );
      state.fa = state.t = state.ea;
      state.eal += state.x;
      state.t += state.x;
      co_await read( state.ea );
      state.m1 = co_await read( state.t );
      state.m2 = ror( state.m1 );
      co_await write( state.ea, state.m2 );
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
      state.a = asl( state.a );
      break;
    case Opcode::IMP_CLC:
      clear<bitC>();
      break;
    case Opcode::IMP_CLD:
      clear<bitD>();
      break;
    case Opcode::IMP_CLI:
      clear<bitI>();
      break;
    case Opcode::IMP_CLV:
      clear<bitV>();
      break;
    case Opcode::IMP_DEC:
      state.a = dec( state.a );
      break;
    case Opcode::IMP_DEX:
      state.x = dec( state.x );
      break;
    case Opcode::IMP_DEY:
      state.y = dec( state.y );
      break;
    case Opcode::IMP_INC:
      state.a = inc( state.a );
      break;
    case Opcode::IMP_INX:
      state.x = inc( state.x );
      break;
    case Opcode::IMP_INY:
      state.y = inc( state.y );
      break;
    case Opcode::IMP_LSR:
      state.a = lsr( state.a );
      break;
    case Opcode::IMP_NOP:
      break;
    case Opcode::IMP_ROL:
      state.a = rol( state.a );
      break;
    case Opcode::IMP_ROR:
      state.a = ror( state.a );
      break;
    case Opcode::IMP_SEC:
      set<bitC>();
      break;
    case Opcode::IMP_SED:
      set<bitD>();
      break;
    case Opcode::IMP_SEI:
      set<bitI>();
      break;
    case Opcode::IMP_TAX:
      setnz( state.x = state.a );
      break;
    case Opcode::IMP_TAY:
      setnz( state.y = state.a );
      break;
    case Opcode::IMP_TSX:
      setnz( state.x = state.sl );
      break;
    case Opcode::IMP_TXA:
      setnz( state.a = state.x );
      break;
    case Opcode::IMP_TXS:
      state.sl = state.x;
      break;
    case Opcode::IMP_TYA:
      setnz( state.a = state.y );
      break;
    case Opcode::IMM_AND:
      ++state.pc;
      setnz( state.a &= state.eal );
      break;
    case Opcode::IMM_BIT:
      ++state.pc;
      setz( state.a & state.eal );
      break;
    case Opcode::IMM_CMP:
      ++state.pc;
      cmp( state.eal );
      break;
    case Opcode::IMM_CPX:
      ++state.pc;
      cpx( state.eal );
      break;
    case Opcode::IMM_CPY:
      ++state.pc;
      cpy( state.eal );
      break;
    case Opcode::IMM_EOR:
      ++state.pc;
      setnz( state.a ^= state.eal );
      break;
    case Opcode::IMM_LDA:
      ++state.pc;
      setnz( state.a = state.eal );
      break;
    case Opcode::IMM_LDX:
      ++state.pc;
      setnz( state.x = state.eal );
      break;
    case Opcode::IMM_LDY:
      ++state.pc;
      setnz( state.y = state.eal );
      break;
    case Opcode::IMM_ORA:
      ++state.pc;
      setnz( state.a |= state.eal );
      break;
    case Opcode::IMM_ADC:
      ++state.pc;
      adc( state.eal );
      if ( get<bitD>() )
      {
        co_await read( state.pc );
      }
      break;
    case Opcode::IMM_SBC:
      ++state.pc;
      sbc( state.eal );
      if ( get<bitD>() )
      {
        co_await read( state.pc );
      }
      break;
    case Opcode::BRL_BCC:
      ++state.pc;
      state.t = state.pc + ( int8_t )state.eal;
      if ( !get<bitC>() )
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
      if ( get<bitC>() )
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
      if ( get<bitZ>() )
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
      if ( get<bitN>() )
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
      if ( !get<bitZ>() )
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
      if ( !get<bitN>() )
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
      if ( !get<bitV>() )
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
      if ( get<bitV>() )
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
        state.eal = co_await read( 0xfffc );
        state.eah = co_await read( 0xfffd );
      }
      else
      {
        //on state.interrupt PC should point to interrupted instruction
        //on BRK should after past BRK argument
        state.pc += state.interrupt ? -1 : 1;
        co_await write( state.s, state.pch );
        state.sl--;
        co_await write( state.s, state.pcl );
        state.sl--;
        co_await write( state.s, getP() );
        state.sl--;
        if ( state.interrupt & CPUState::I_NMI )
        {
          state.eal = co_await read( 0xfffa );
          state.eah = co_await read( 0xfffb );
        }
        else
        {
          state.eal = co_await read( 0xfffe );
          state.eah = co_await read( 0xffff );
        }
        set<bitI>();
      }
      clear<bitD>();
      state.pc = state.ea;
      break;
    case Opcode::RTI_RTI:
      ++state.pc;
      ++state.sl;
      setP( co_await read( state.s ) );
      ++state.sl;
      state.eal = co_await read( state.s );
      ++state.sl;
      state.eah = co_await read( state.s );
      co_await read( state.pc );
      state.pc = state.ea;
      break;
    case Opcode::RTS_RTS:
      co_await read( ++state.pc );
      ++state.sl;
      state.eal = co_await read( state.s );
      ++state.sl;
      state.eah = co_await read( state.s );
      co_await read( state.pc );
      ++state.ea;
      state.pc = state.ea;
      break;
    case Opcode::PHR_PHA:
      co_await write( state.s, state.a );
      state.sl--;
      break;
    case Opcode::PHR_PHP:
      co_await write( state.s, getP() );
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
      setnz( state.a = co_await read( state.s ) );
      break;
    case Opcode::PLR_PLP:
      co_await read( state.pc );
      ++state.sl;
      setP( co_await read( state.s ) );
      break;
    case Opcode::PLR_PLX:
      co_await read( state.pc );
      ++state.sl;
      setnz( state.x = co_await read( state.s ) );
      break;
    case Opcode::PLR_PLY:
      co_await read( state.pc );
      ++state.sl;
      setnz( state.y = co_await read( state.s ) );
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
      co_await discardRead( state.ea );
      break;
    case Opcode::UND_4_54:
    case Opcode::UND_4_d4:
    case Opcode::UND_4_f4:
      ++state.pc;
      co_await read( state.pc );
      state.tl = state.eal + state.x;
      co_await discardRead( state.ea );
      break;
    case Opcode::UND_4_dc:
    case Opcode::UND_4_fc:
      ++state.pc;
      state.eah = co_await read( state.pc++ );
      co_await discardRead( state.ea );
      break;
    case Opcode::UND_8_5c:
      //https://laughtonelectronics.com/Arcana/KimKlone/Kimklone_opcode_mapping.html
      //state.op - code 5C consumes 3 bytes and 8 cycles but conforms to no known address mode; it remains interesting but useless.
      //I tested the instruction "5C 1234h" ( stored little - endian as 5Ch 34h 12h ) as an example, and observed the following : 3 cycles fetching the instruction, 1 cycle reading FF34, then 4 cycles reading FFFF.
      ++state.pc;
      co_await read( state.pc++ );
      state.eah = 0xff;
      co_await discardRead( state.ea );
      co_await discardRead( 0xffff );
      co_await discardRead( 0xffff );
      co_await discardRead( 0xffff );
      co_await discardRead( 0xffff );
      break;
    default:  //for UND_1_xx
      break;
    }

    trace2();

    do
    {
      trace1();
      co_await fetchOpcode( state.pc++ );
    } while ( isHiccup() );
  }

}

uint8_t CPU::inc( uint8_t val )
{
  uint8_t result = val + 1;
  setnz( result );
  return result;
}

uint8_t CPU::dec( uint8_t val )
{
  uint8_t result = val - 1;
  setnz( result );
  return result;
}

uint8_t CPU::asl( uint8_t val )
{
  set<bitC>( val >= 0x80 );
  uint8_t result = val << 1;
  setnz( result );
  return result;
}

uint8_t CPU::lsr( uint8_t val )
{
  set<bitC>( ( val & 0x01 ) != 0 );
  uint8_t result = val >> 1;
  setnz( result );
  return result;
}

uint8_t CPU::rol( uint8_t val )
{
  int roled = val << 1;
  uint8_t result = roled & 0xff | ( get<bitC>() ? 0x01 : 0 );
  setnz( result );
  set<bitC>( ( roled & 0x100 ) != 0 );
  return result;
}

uint8_t CPU::ror( uint8_t val )
{
  bool newC = ( val & 1 ) != 0;
  uint8_t result = ( val >> 1 ) | ( get<bitC>() ? 0x80 : 0 );
  setnz( result );
  set<bitC>( newC );
  return result;
}

void CPU::adc( uint8_t value )
{
  if ( get<bitD>() )
  {
    int lo = ( mState.a & 0x0f ) + ( value & 0x0f ) + ( get<bitC>() ? 0x01 : 0 );
    int hi = ( mState.a & 0xf0 ) + ( value & 0xf0 );
    clear<bitV>();
    clear<bitC>();
    if ( lo > 0x09 )
    {
      hi += 0x10;
      lo += 0x06;
    }
    if ( ~( mState.a ^ value ) & ( mState.a ^ hi ) & 0x80 )
    {
      set<bitV>();
    }
    if ( hi > 0x90 )
    {
      hi += 0x60;
    }
    if ( hi & 0xff00 )
    {
      set<bitC>();
    }
    mState.a = ( lo & 0x0f ) + ( hi & 0xf0 );
    setnz( mState.a );
  }
  else
  {
    int sum = mState.a + value + ( get<bitC>() ? 0x01 : 0 );
    clear<bitV>();
    clear<bitC>();
    if ( ~( mState.a ^ value ) & ( mState.a ^ sum ) & 0x80 )
    {
      set<bitV>();
    }
    if ( sum & 0xff00 )
    {
      set<bitC>();
    }
    mState.a = (uint8_t)sum;
    setnz( mState.a );
  }
}

void CPU::sbc( uint8_t value )
{
  if ( get<bitD>() )
  {
    int c = get<bitC>() ? 0 : 1;
    int sum = mState.a - value - c;
    int lo = ( mState.a & 0x0f ) - ( value & 0x0f ) - c;
    int hi = ( mState.a & 0xf0 ) - ( value & 0xf0 );
    clear<bitV>();
    clear<bitC>();
    if ( ( mState.a ^ value ) & ( mState.a ^ sum ) & 0x80 )
    {
      set<bitV>();
    }
    if ( lo & 0xf0 )
    {
      lo -= 6;
    }
    if ( lo & 0x80 )
    {
      hi -= 0x10;
    }
    if ( hi & 0x0f00 )
    {
      hi -= 0x60;
    }
    if ( ( sum & 0xff00 ) == 0 )
    {
      set<bitC>();
    }
    mState.a = ( lo & 0x0f ) + ( hi & 0xf0 );
    setnz( mState.a );
  }
  else
  {
    int c = get<bitC>() ? 0 : 1;
    int sum = mState.a - value - c;
    clear<bitV>();
    clear<bitC>();
    if ( ( mState.a ^ value ) & ( mState.a ^ sum ) & 0x80 )
    {
      set<bitV>();
    }
    if ( ( sum & 0xff00 ) == 0 )
    {
      set<bitC>();
    }
    mState.a = (uint8_t)sum;
    setnz( mState.a );
  }
}

void CPU::bit( uint8_t value )
{
  setz( mState.a & value );
  set<bitN>( ( value & 0x80 ) != 0x00 );
  set<bitV>( ( value & 0x40 ) != 0x00 );
}

void CPU::cmp( uint8_t value )
{
  set<bitC>( mState.a >= value );
  setnz( mState.a - value );;
}

void CPU::cpx( uint8_t value )
{
  set<bitC>( mState.x >= value );
  setnz( mState.x - value );
}

void CPU::cpy( uint8_t value )
{
  set<bitC>( mState.y >= value );
  setnz( mState.y - value );
}

void CPU::trace1()
{
  if ( !mTrace )
    return;

  off = sprintf( buf.data(), "%llu: PC:%04x A:%02x X:%02x Y:%02x S:%04x P:%c%c1%c%c%c%c%c ", mState.tick, mState.pc, mState.a, mState.x, mState.y, mState.s, ( CPU::get<CPU::bitN>( mState.p ) ? 'N' : '-' ), ( CPU::get<CPU::bitV>( mState.p ) ? 'V' : '-' ), ( CPU::get<CPU::bitB>( mState.p ) ? 'B' : '-' ), ( CPU::get<CPU::bitD>( mState.p ) ? 'D' : '-' ), ( CPU::get<CPU::bitI>( mState.p ) ? 'I' : '-' ), ( CPU::get<CPU::bitZ>( mState.p ) ? 'Z' : '-' ), ( CPU::get<CPU::bitC>( mState.p ) ? 'C' : '-' ) );
}

void CPU::trace2()
{
  if ( !mTrace )
    return;

  switch ( mState.op )
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
    off += sprintf( buf.data() + off, "and " );
    break;
  case Opcode::RZP_BIT:
  case Opcode::RZX_BIT:
  case Opcode::RAB_BIT:
  case Opcode::RAX_BIT:
  case Opcode::IMM_BIT:
    off += sprintf( buf.data() + off, "bit " );
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
    off += sprintf( buf.data() + off, "cmp " );
    break;
  case Opcode::RZP_CPX:
  case Opcode::RAB_CPX:
  case Opcode::IMM_CPX:
    off += sprintf( buf.data() + off, "cpx " );
    break;
  case Opcode::RZP_CPY:
  case Opcode::RAB_CPY:
  case Opcode::IMM_CPY:
    off += sprintf( buf.data() + off, "cpy " );
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
    off += sprintf( buf.data() + off, "eor " );
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
    off += sprintf( buf.data() + off, "lda " );
    break;
  case Opcode::RZP_LDX:
  case Opcode::RZY_LDX:
  case Opcode::RAB_LDX:
  case Opcode::RAY_LDX:
  case Opcode::IMM_LDX:
    off += sprintf( buf.data() + off, "ldx " );
    break;
  case Opcode::RZP_LDY:
  case Opcode::RZX_LDY:
  case Opcode::RAB_LDY:
  case Opcode::RAX_LDY:
  case Opcode::IMM_LDY:
    off += sprintf( buf.data() + off, "ldy " );
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
    off += sprintf( buf.data() + off, "ora " );
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
    off += sprintf( buf.data() + off, "adc " );
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
    off += sprintf( buf.data() + off, "sbc " );
    break;
  case Opcode::WZP_STA:
  case Opcode::WZX_STA:
  case Opcode::WIN_STA:
  case Opcode::WIX_STA:
  case Opcode::WIY_STA:
  case Opcode::WAB_STA:
  case Opcode::WAX_STA:
  case Opcode::WAY_STA:
    off += sprintf( buf.data() + off, "sta " );
    break;
  case Opcode::WZP_STX:
  case Opcode::WZY_STX:
  case Opcode::WAB_STX:
    off += sprintf( buf.data() + off, "stx " );
    break;
  case Opcode::WZP_STY:
  case Opcode::WZX_STY:
  case Opcode::WAB_STY:
    off += sprintf( buf.data() + off, "sty " );
    break;
  case Opcode::WZP_STZ:
  case Opcode::WZX_STZ:
  case Opcode::WAB_STZ:
  case Opcode::WAX_STZ:
    off += sprintf( buf.data() + off, "stz " );
    break;
  case Opcode::MZP_ASL:
  case Opcode::MZX_ASL:
  case Opcode::MAB_ASL:
  case Opcode::MAX_ASL:
    off += sprintf( buf.data() + off, "asl " );
    break;
  case Opcode::IMP_ASL:
    off += sprintf( buf.data() + off, "asl\n" );
    break;
  case Opcode::MZP_DEC:
  case Opcode::MZX_DEC:
  case Opcode::MAB_DEC:
  case Opcode::MAX_DEC:
    off += sprintf( buf.data() + off, "dec " );
    break;
  case Opcode::IMP_DEC:
    off += sprintf( buf.data() + off, "dec\n" );
    break;
  case Opcode::MZP_INC:
  case Opcode::MZX_INC:
  case Opcode::MAB_INC:
  case Opcode::MAX_INC:
    off += sprintf( buf.data() + off, "inc " );
    break;
  case Opcode::IMP_INC:
    off += sprintf( buf.data() + off, "inc\n" );
    break;
  case Opcode::MZP_LSR:
  case Opcode::MZX_LSR:
  case Opcode::MAB_LSR:
  case Opcode::MAX_LSR:
    off += sprintf( buf.data() + off, "lsr " );
    break;
  case Opcode::IMP_LSR:
    off += sprintf( buf.data() + off, "lsr\n" );
    break;
  case Opcode::MZP_ROL:
  case Opcode::MZX_ROL:
  case Opcode::MAB_ROL:
  case Opcode::MAX_ROL:
    off += sprintf( buf.data() + off, "rol " );
    break;
  case Opcode::IMP_ROL:
    off += sprintf( buf.data() + off, "rol\n" );
    break;
  case Opcode::MZP_ROR:
  case Opcode::MZX_ROR:
  case Opcode::MAB_ROR:
  case Opcode::MAX_ROR:
    off += sprintf( buf.data() + off, "ror " );
    break;
  case Opcode::IMP_ROR:
    off += sprintf( buf.data() + off, "ror\n" );
    break;
  case Opcode::MZP_TRB:
  case Opcode::MAB_TRB:
    off += sprintf( buf.data() + off, "trb " );
    break;
  case Opcode::MZP_TSB:
  case Opcode::MAB_TSB:
    off += sprintf( buf.data() + off, "tsb " );
    break;
  case Opcode::MZP_RMB0:
    off += sprintf( buf.data() + off, "rmb0 " );
    break;
  case Opcode::MZP_RMB1:
    off += sprintf( buf.data() + off, "rmb1 " );
    break;
  case Opcode::MZP_RMB2:
    off += sprintf( buf.data() + off, "rmb2 " );
    break;
  case Opcode::MZP_RMB3:
    off += sprintf( buf.data() + off, "rmb3 " );
    break;
  case Opcode::MZP_RMB4:
    off += sprintf( buf.data() + off, "rmb4 " );
    break;
  case Opcode::MZP_RMB5:
    off += sprintf( buf.data() + off, "rmb5 " );
    break;
  case Opcode::MZP_RMB6:
    off += sprintf( buf.data() + off, "rmb6 " );
    break;
  case Opcode::MZP_RMB7:
    off += sprintf( buf.data() + off, "rmb7 " );
    break;
  case Opcode::MZP_SMB0:
    off += sprintf( buf.data() + off, "smb0 " );
    break;
  case Opcode::MZP_SMB1:
    off += sprintf( buf.data() + off, "smb1 " );
    break;
  case Opcode::MZP_SMB2:
    off += sprintf( buf.data() + off, "smb2 " );
    break;
  case Opcode::MZP_SMB3:
    off += sprintf( buf.data() + off, "smb3 " );
    break;
  case Opcode::MZP_SMB4:
    off += sprintf( buf.data() + off, "smb4 " );
    break;
  case Opcode::MZP_SMB5:
    off += sprintf( buf.data() + off, "smb5 " );
    break;
  case Opcode::MZP_SMB6:
    off += sprintf( buf.data() + off, "smb6 " );
    break;
  case Opcode::MZP_SMB7:
    off += sprintf( buf.data() + off, "smb7 " );
    break;
  case Opcode::JMA_JMP:
  case Opcode::JMX_JMP:
  case Opcode::JMI_JMP:
    off += sprintf( buf.data() + off, "jmp " );
    break;
  case Opcode::JSA_JSR:
    off += sprintf( buf.data() + off, "jsr " );
    break;
  case Opcode::IMP_CLC:
    off += sprintf( buf.data() + off, "clc\n" );
    break;
  case Opcode::IMP_CLD:
    off += sprintf( buf.data() + off, "cld\n" );
    break;
  case Opcode::IMP_CLI:
    off += sprintf( buf.data() + off, "cli\n" );
    break;
  case Opcode::IMP_CLV:
    off += sprintf( buf.data() + off, "clv\n" );
    break;
  case Opcode::IMP_DEX:
    off += sprintf( buf.data() + off, "dex\n" );
    break;
  case Opcode::IMP_DEY:
    off += sprintf( buf.data() + off, "dey\n" );
    break;
  case Opcode::IMP_INX:
    off += sprintf( buf.data() + off, "inx\n" );
    break;
  case Opcode::IMP_INY:
    off += sprintf( buf.data() + off, "iny\n" );
    break;
  case Opcode::IMP_NOP:
    off += sprintf( buf.data() + off, "nop\n" );
    break;
  case Opcode::UND_2_02:
  case Opcode::UND_2_22:
  case Opcode::UND_2_42:
  case Opcode::UND_2_62:
  case Opcode::UND_2_82:
  case Opcode::UND_2_C2:
  case Opcode::UND_2_E2:
    off += sprintf( buf.data() + off, "nop " );
    break;
  case Opcode::IMP_SEC:
    off += sprintf( buf.data() + off, "sec\n" );
    break;
  case Opcode::IMP_SED:
    off += sprintf( buf.data() + off, "sed\n" );
    break;
  case Opcode::IMP_SEI:
    off += sprintf( buf.data() + off, "sei\n" );
    break;
  case Opcode::IMP_TAX:
    off += sprintf( buf.data() + off, "tax\n" );
    break;
  case Opcode::IMP_TAY:
    off += sprintf( buf.data() + off, "tay\n" );
    break;
  case Opcode::IMP_TSX:
    off += sprintf( buf.data() + off, "tsx\n" );
    break;
  case Opcode::IMP_TXA:
    off += sprintf( buf.data() + off, "txa\n" );
    break;
  case Opcode::IMP_TXS:
    off += sprintf( buf.data() + off, "txs\n" );
    break;
  case Opcode::IMP_TYA:
    off += sprintf( buf.data() + off, "tya\n" );
    break;
  case Opcode::BRL_BCC:
    off += sprintf( buf.data() + off, "bcc " );
    break;
  case Opcode::BRL_BCS:
    off += sprintf( buf.data() + off, "bcs " );
    break;
  case Opcode::BRL_BEQ:
    off += sprintf( buf.data() + off, "beq " );
    break;
  case Opcode::BRL_BMI:
    off += sprintf( buf.data() + off, "bmi " );
    break;
  case Opcode::BRL_BNE:
    off += sprintf( buf.data() + off, "bne " );
    break;
  case Opcode::BRL_BPL:
    off += sprintf( buf.data() + off, "bpl " );
    break;
  case Opcode::BRL_BRA:
    off += sprintf( buf.data() + off, "bra " );
    break;
  case Opcode::BRL_BVC:
    off += sprintf( buf.data() + off, "bvc " );
    break;
  case Opcode::BRL_BVS:
    off += sprintf( buf.data() + off, "bvs " );
    break;
  case Opcode::BZR_BBR0:
    off += sprintf( buf.data() + off, "bbr0 " );
    break;
  case Opcode::BZR_BBR1:
    off += sprintf( buf.data() + off, "bbr1 " );
    break;
  case Opcode::BZR_BBR2:
    off += sprintf( buf.data() + off, "bbr2 " );
    break;
  case Opcode::BZR_BBR3:
    off += sprintf( buf.data() + off, "bbr3 " );
    break;
  case Opcode::BZR_BBR4:
    off += sprintf( buf.data() + off, "bbr4 " );
    break;
  case Opcode::BZR_BBR5:
    off += sprintf( buf.data() + off, "bbr5 " );
    break;
  case Opcode::BZR_BBR6:
    off += sprintf( buf.data() + off, "bbr6 " );
    break;
  case Opcode::BZR_BBR7:
    off += sprintf( buf.data() + off, "bbr7 " );
    break;
  case Opcode::BZR_BBS0:
    off += sprintf( buf.data() + off, "bbs0 " );
    break;
  case Opcode::BZR_BBS1:
    off += sprintf( buf.data() + off, "bbs1 " );
    break;
  case Opcode::BZR_BBS2:
    off += sprintf( buf.data() + off, "bbs2 " );
    break;
  case Opcode::BZR_BBS3:
    off += sprintf( buf.data() + off, "bbs3 " );
    break;
  case Opcode::BZR_BBS4:
    off += sprintf( buf.data() + off, "bbs4 " );
    break;
  case Opcode::BZR_BBS5:
    off += sprintf( buf.data() + off, "bbs5 " );
    break;
  case Opcode::BZR_BBS6:
    off += sprintf( buf.data() + off, "bbs6 " );
    break;
  case Opcode::BZR_BBS7:
    off += sprintf( buf.data() + off, "bbs7 " );
    break;
  case Opcode::BRK_BRK:
    if ( ( mState.interrupt & CPUState::I_RESET ) != 0 )
    {
      off += sprintf( buf.data() + off, "RESET\n" );
    }
    else if ( ( mState.interrupt & CPUState::I_NMI ) != 0 )
    {
      off += sprintf( buf.data() + off, "NMI\n" );
    }
    else if ( ( mState.interrupt & CPUState::I_IRQ ) != 0 )
    {
      off += sprintf( buf.data() + off, "IRQ\n" );
    }
    else
    {
      off += sprintf( buf.data() + off, "brk\n" );
    }
    break;
  case Opcode::RTI_RTI:
    off += sprintf( buf.data() + off, "rti\n" );
    break;
  case Opcode::RTS_RTS:
    off += sprintf( buf.data() + off, "rts\n" );
    break;
  case Opcode::PHR_PHA:
    off += sprintf( buf.data() + off, "pha\n" );
    break;
  case Opcode::PHR_PHP:
    off += sprintf( buf.data() + off, "php\n" );
    break;
  case Opcode::PHR_PHX:
    off += sprintf( buf.data() + off, "phx\n" );
    break;
  case Opcode::PHR_PHY:
    off += sprintf( buf.data() + off, "phy\n" );
    break;
  case Opcode::PLR_PLA:
    off += sprintf( buf.data() + off, "pla\n" );
    break;
  case Opcode::PLR_PLP:
    off += sprintf( buf.data() + off, "plp\n" );
    break;
  case Opcode::PLR_PLX:
    off += sprintf( buf.data() + off, "plx\n" );
    break;
  case Opcode::PLR_PLY:
    off += sprintf( buf.data() + off, "ply\n" );
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
    off += sprintf( buf.data() + off, "nop\n" );
    break;
  case Opcode::UND_3_44:
  case Opcode::UND_4_54:
  case Opcode::UND_4_d4:
  case Opcode::UND_4_f4:
  case Opcode::UND_4_dc:
  case Opcode::UND_4_fc:
  case Opcode::UND_8_5c:
    off += sprintf( buf.data() + off, "nop " );
    break;
  }


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
    sprintf( buf.data() + off, "$%02x\t;$%02x\n", mState.eal, mState.m1 );
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
    sprintf( buf.data() + off, "$%02x\t;$%02x->$%02x\n", mState.eal, mState.m1, mState.m2 );
    break;
  case Opcode::WZP_STA:
  case Opcode::WZP_STX:
  case Opcode::WZP_STY:
  case Opcode::WZP_STZ:
  case Opcode::UND_3_44:
    sprintf( buf.data() + off, "$%02x\n", mState.eal );
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
    sprintf( buf.data() + off, "$%02x,x\t;[$%04x]=$%02x\n", mState.eal, mState.t, mState.m1 );
    break;
  case Opcode::MZX_ASL:
  case Opcode::MZX_DEC:
  case Opcode::MZX_INC:
  case Opcode::MZX_LSR:
  case Opcode::MZX_ROL:
  case Opcode::MZX_ROR:
    sprintf( buf.data() + off, "$%02x,x\t;[$%04x]=$%02x->$%02x\n", mState.eal, mState.t, mState.m1, mState.m2 );
    break;
  case Opcode::WZX_STA:
  case Opcode::WZX_STY:
  case Opcode::WZX_STZ:
  case Opcode::UND_4_54:
  case Opcode::UND_4_d4:
  case Opcode::UND_4_f4:
    sprintf( buf.data() + off, "$%02x,x\t;[$%04x]\n", mState.eal, mState.t );
    break;
  case Opcode::RZY_LDX:
    sprintf( buf.data() + off, "$%02x,y\t;[$%04x]=$%02x\n", mState.eal, mState.t, mState.m1 );
    break;
  case Opcode::WZY_STX:
    sprintf( buf.data() + off, "$%02x,y\t;[$%04x]\n", mState.eal, mState.t );
    break;
  case Opcode::RIN_LDA:
  case Opcode::RIN_AND:
  case Opcode::RIN_CMP:
  case Opcode::RIN_EOR:
  case Opcode::RIN_ORA:
  case Opcode::RIN_ADC:
  case Opcode::RIN_SBC:
    sprintf( buf.data() + off, "($%02x)\t;[$%04x]=$%02x\n", mState.fa, mState.t, mState.m1 );
    break;
  case Opcode::WIN_STA:
    sprintf( buf.data() + off, "($%02x)\t;[$%04x]\n", mState.eal, mState.t );
    break;
  case Opcode::RIX_AND:
  case Opcode::RIX_CMP:
  case Opcode::RIX_EOR:
  case Opcode::RIX_LDA:
  case Opcode::RIX_ORA:
  case Opcode::RIX_ADC:
  case Opcode::RIX_SBC:
    sprintf( buf.data() + off, "($%02x,x)\t;[$%04x]=$%02x\n", mState.fa, mState.t, mState.m1 );
    break;
  case Opcode::WIX_STA:
    sprintf( buf.data() + off, "($%02x,x)\t;[$%04x]\n", mState.fa, mState.t );
    break;
  case Opcode::RIY_AND:
  case Opcode::RIY_CMP:
  case Opcode::RIY_EOR:
  case Opcode::RIY_LDA:
  case Opcode::RIY_ORA:
  case Opcode::RIY_ADC:
  case Opcode::RIY_SBC:
    sprintf( buf.data() + off, "($%02x),y\t;[$%04x]=$%02x\n", mState.fa, mState.ea, mState.m1 );
    break;
  case Opcode::WIY_STA:
    sprintf( buf.data() + off, "($%02x),y\t;[$%04x]\n", mState.fa, mState.ea );
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
    sprintf( buf.data() + off, "$%04x\t;=$%02x\n", mState.ea, mState.m1 );
    break;
  case Opcode::MAB_ASL:
  case Opcode::MAB_DEC:
  case Opcode::MAB_INC:
  case Opcode::MAB_LSR:
  case Opcode::MAB_ROL:
  case Opcode::MAB_ROR:
  case Opcode::MAB_TRB:
  case Opcode::MAB_TSB:
    sprintf( buf.data() + off, "$%04x\t;=$%02x->$%02x\n", mState.ea, mState.m1, mState.m2 );
    break;
  case Opcode::WAB_STA:
  case Opcode::WAB_STX:
  case Opcode::WAB_STY:
  case Opcode::WAB_STZ:
  case Opcode::JMA_JMP:
  case Opcode::JSA_JSR:
  case Opcode::UND_4_dc:
  case Opcode::UND_4_fc:
  case Opcode::UND_8_5c:
    sprintf( buf.data() + off, "$%04x\n", mState.ea );
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
    sprintf( buf.data() + off, "$%04x,x\t;[$%04x]=$%02x\n", mState.fa, mState.t, mState.m1 );
    break;
  case Opcode::MAX_ASL:
  case Opcode::MAX_DEC:
  case Opcode::MAX_INC:
  case Opcode::MAX_LSR:
  case Opcode::MAX_ROL:
  case Opcode::MAX_ROR:
    sprintf( buf.data() + off, "$%04x,x\t;[$%04x]=$%02x->$%02x\n", mState.fa, mState.t, mState.m1, mState.m2 );
    break;
  case Opcode::WAX_STA:
  case Opcode::WAX_STZ:
    sprintf( buf.data() + off, "$%04x,x\t;[$%04x]\n", mState.fa, mState.t );
    break;
  case Opcode::RAY_AND:
  case Opcode::RAY_CMP:
  case Opcode::RAY_EOR:
  case Opcode::RAY_LDA:
  case Opcode::RAY_LDX:
  case Opcode::RAY_ORA:
  case Opcode::RAY_ADC:
  case Opcode::RAY_SBC:
    sprintf( buf.data() + off, "$%04x,y\t;[$%04x]=$%02x\n", mState.fa, mState.t, mState.m1 );
    break;
  case Opcode::WAY_STA:
    sprintf( buf.data() + off, "$%04x,y\t;[$%04x]\n", mState.fa, mState.t );
    break;
  case Opcode::JMX_JMP:
    sprintf( buf.data() + off, "($%04x,x)\t;[$%04x]\n", mState.fa, mState.ea );
    break;
  case Opcode::JMI_JMP:
    sprintf( buf.data() + off, "($%04x)\t;[$%04x]\n", mState.fa, mState.t );
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
  case Opcode::BRK_BRK:
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
    sprintf( buf.data() + off, "#$%02x\n", mState.eal );
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
    sprintf( buf.data() + off, "$%04x\n", mState.t );
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
    sprintf( buf.data() + off, "$%02x,$%04x\t;$%02x\n", mState.eal, mState.t, mState.m1 );
    break;
  }

  mFtrace << buf.data();

}

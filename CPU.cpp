#include "CPU.hpp"
#include "BusMaster.hpp"
#include "Opcodes.hpp"

bool isHiccup( Opcode opcode )
{
  switch ( opcode )
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
    return true;
  default:
    return false;
  }
}

CpuLoop cpuLoop( CPU & cpu )
{
  OpInt opint{ Opcode{}, CPU::I_RESET };
  for ( ;; )
  {
    union
    {
      uint16_t ea{};
      struct
      {
        uint8_t eal;
        uint8_t eah;
      };
    };

    union
    {
      uint16_t t;
      struct
      {
        uint8_t tl;
        uint8_t th;
      };
    };

    uint8_t d;

    if ( opint.interrupt && ( !cpu.I || ( opint.interrupt & ~CPU::I_IRQ ) != 0 ) )
    {
      opint.op = Opcode::BRK_BRK;
    }

    eal = co_yield{ cpu.pc++ };

    switch ( opint.op )
    {
    case Opcode::RZP_AND:
    case Opcode::RZP_BIT:
    case Opcode::RZP_CMP:
    case Opcode::RZP_CPX:
    case Opcode::RZP_CPY:
    case Opcode::RZP_EOR:
    case Opcode::RZP_LDA:
    case Opcode::RZP_LDX:
    case Opcode::RZP_LDY:
    case Opcode::RZP_ORA:
      d = co_yield{ ea };
      cpu.executeR( opint.op, d );
      break;
    case Opcode::RZP_ADC:
    case Opcode::RZP_SBC:
      d = co_yield{ ea };
      if ( cpu.executeR( opint.op, d ) )
      {
        co_yield{ ea };
      }
      break;
    case Opcode::WZP_STA:
      co_yield{ ea, cpu.a };
      break;
    case Opcode::WZP_STX:
      co_yield{ ea, cpu.x };
      break;
    case Opcode::WZP_STY:
      co_yield{ ea, cpu.y };
      break;
    case Opcode::WZP_STZ:
      co_yield{ ea, 0x00 };
      break;
    case Opcode::MZP_ASL:
      d = co_yield{ ea };
      cpu.asl( d );
      co_yield{ ea, d };
      break;
    case Opcode::MZP_DEC:
      d = co_yield{ ea };
      cpu.setnz( --d );
      co_yield{ ea, d };
      break;
    case Opcode::MZP_INC:
      d = co_yield{ ea };
      cpu.setnz( ++d );
      co_yield{ ea, d };
      break;
    case Opcode::MZP_LSR:
      d = co_yield{ ea };
      cpu.lsr( d );
      co_yield{ ea, d };
      break;
    case Opcode::MZP_ROL:
      d = co_yield{ ea };
      cpu.rol( d );
      co_yield{ ea, d };
      break;
    case Opcode::MZP_ROR:
      cpu.ror( d );
      co_yield{ ea, d };
      break;
    case Opcode::MZP_TRB:
      d = co_yield{ ea };
      cpu.setz( d &= ~cpu.a );
      co_yield{ ea, d };
      break;
    case Opcode::MZP_TSB:
      d = co_yield{ ea };
      cpu.setz( d |= cpu.a );
      co_yield{ ea, d };
      break;
    case Opcode::MZP_RMB0:
      d = co_yield{ ea };
      d &= 0b11111110;
      co_yield{ ea, d };
      break;
    case Opcode::MZP_RMB1:
      d = co_yield{ ea };
      d &= 0b11111101;
      co_yield{ ea, d };
      break;
    case Opcode::MZP_RMB2:
      d &= 0b11111011;
      co_yield{ ea, d };
      break;
    case Opcode::MZP_RMB3:
      d = co_yield{ ea };
      d &= 0b11110111;
      co_yield{ ea, d };
      break;
    case Opcode::MZP_RMB4:
      d = co_yield{ ea };
      d &= 0b11101111;
      co_yield{ ea, d };
      break;
    case Opcode::MZP_RMB5:
      d = co_yield{ ea };
      d &= 0b11011111;
      co_yield{ ea, d };
      break;
    case Opcode::MZP_RMB6:
      d = co_yield{ ea };
      d &= 0b10111111;
      co_yield{ ea, d };
      break;
    case Opcode::MZP_RMB7:
      d = co_yield{ ea };
      d &= 0b01111111;
      co_yield{ ea, d };
      break;
    case Opcode::MZP_SMB0:
      d = co_yield{ ea };
      d |= 0b00000001;
      co_yield{ ea, d };
      break;
    case Opcode::MZP_SMB1:
      d = co_yield{ ea };
      d |= 0b00000010;
      co_yield{ ea, d };
      break;
    case Opcode::MZP_SMB2:
      d = co_yield{ ea };
      d |= 0b00000100;
      co_yield{ ea, d };
      break;
    case Opcode::MZP_SMB3:
      d = co_yield{ ea };
      d |= 0b00001000;
      co_yield{ ea, d };
      break;
    case Opcode::MZP_SMB4:
      d = co_yield{ ea };
      d |= 0b00010000;
      co_yield{ ea, d };
      break;
    case Opcode::MZP_SMB5:
      d = co_yield{ ea };
      d |= 0b00100000;
      co_yield{ ea, d };
      break;
    case Opcode::MZP_SMB6:
      d = co_yield{ ea };
      d |= 0b01000000;
      co_yield{ ea, d };
      break;
    case Opcode::MZP_SMB7:
      d = co_yield{ ea };
      d |= 0b10000000;
      co_yield{ ea, d };
      break;
    break;
    case Opcode::RZX_AND:
    case Opcode::RZX_BIT:
    case Opcode::RZX_CMP:
    case Opcode::RZX_EOR:
    case Opcode::RZX_LDA:
    case Opcode::RZX_LDY:
    case Opcode::RZX_ORA:
      co_yield{ cpu.pc };
      eal += cpu.x;
      d = co_yield{ ea };
      cpu.executeR( opint.op, d );
      break;
    case Opcode::RZX_ADC:
    case Opcode::RZX_SBC:
      co_yield{ cpu.pc };
      eal += cpu.x;
      d = co_yield{ ea };
      if ( cpu.executeR( opint.op, d ) )
      {
        co_yield{ ea };
      }
      break;
    case Opcode::RZY_LDX:
      co_yield{ cpu.pc };
      eal += cpu.y;
      cpu.setnz( cpu.a = co_yield{ ea } );
      break;
    case Opcode::WZX_STA:
      co_yield{ cpu.pc };
      eal += cpu.x;
      co_yield{ ea, cpu.a };
      break;
    case Opcode::WZX_STY:
      co_yield{ cpu.pc };
      eal += cpu.x;
      co_yield{ ea, cpu.y };
      break;
    case Opcode::WZX_STZ:
      co_yield{ cpu.pc };
      eal += cpu.x;
      co_yield{ ea, 0x00 };
      break;
    case Opcode::WZY_STX:
      co_yield{ cpu.pc };
      eal += cpu.y;
      co_yield{ ea, cpu.x };
      break;
    case Opcode::MZX_ASL:
      co_yield{ cpu.pc };
      eal += cpu.x;
      d = co_yield{ ea };
      cpu.asl( d );
      co_yield{ ea, d };
      break;
    case Opcode::MZX_DEC:
      co_yield{ cpu.pc };
      eal += cpu.x;
      d = co_yield{ ea };
      cpu.setnz( --d );
      co_yield{ ea, d };
      break;
    case Opcode::MZX_INC:
      co_yield{ cpu.pc };
      eal += cpu.x;
      d = co_yield{ ea };
      cpu.setnz( ++d );
      co_yield{ ea, d };
      break;
    case Opcode::MZX_LSR:
      co_yield{ cpu.pc };
      eal += cpu.x;
      d = co_yield{ ea };
      cpu.lsr( d );
      co_yield{ ea, d };
      break;
    case Opcode::MZX_ROL:
      co_yield{ cpu.pc };
      eal += cpu.x;
      d = co_yield{ ea };
      cpu.rol( d );
      co_yield{ ea, d };
      break;
    case Opcode::MZX_ROR:
      co_yield{ cpu.pc };
      eal += cpu.x;
      d = co_yield{ ea };
      cpu.ror( d );
      co_yield{ ea, d };
      break;
    case Opcode::RIN_AND:
    case Opcode::RIN_CMP:
    case Opcode::RIN_EOR:
    case Opcode::RIN_LDA:
    case Opcode::RIN_ORA:
      tl = co_yield{ ea++ };
      th = co_yield{ ea };
      d = co_yield{ t };
      cpu.executeR( opint.op, d );
      break;
    case Opcode::RIN_ADC:
    case Opcode::RIN_SBC:
      tl = co_yield{ ea++ };
      th = co_yield{ ea };
      d = co_yield{ t };
      if ( cpu.executeR( opint.op, d ) )
      {
        co_yield{ t };
      }
      break;
    case Opcode::WIN_STA:
      tl = co_yield{ ea++ };
      th = co_yield{ ea };
      co_yield{ t, cpu.a };
      break;
    case Opcode::RIX_AND:
    case Opcode::RIX_CMP:
    case Opcode::RIX_EOR:
    case Opcode::RIX_LDA:
    case Opcode::RIX_ORA:
      co_yield{ cpu.pc };
      eal += cpu.x;
      tl = co_yield{ ea++ };
      th = co_yield{ ea };
      d = co_yield{ t };
      cpu.executeR( opint.op, d );
      break;
    case Opcode::RIX_ADC:
    case Opcode::RIX_SBC:
      co_yield{ cpu.pc };
      eal += cpu.x;
      tl = co_yield{ ea++ };
      th = co_yield{ ea };
      d = co_yield{ t };
      if ( cpu.executeR( opint.op, d ) )
      {
        co_yield{ t };
      }
      break;
    case Opcode::WIX_STA:
      co_yield{ cpu.pc };
      eal += cpu.x;
      tl = co_yield{ ea++ };
      th = co_yield{ ea };
      co_yield{ t, cpu.a };
      break;
    case Opcode::RIY_AND:
    case Opcode::RIY_CMP:
    case Opcode::RIY_EOR:
    case Opcode::RIY_LDA:
    case Opcode::RIY_ORA:
      co_yield{ cpu.pc };
      tl = co_yield{ ea++ };
      th = co_yield{ ea };
      ea = t;
      eal += cpu.y;
      if ( eah != th )
      {
        co_yield{ ea };
      }
      d = co_yield{ t };
      cpu.executeR( opint.op, d );
      break;
    case Opcode::RIY_ADC:
    case Opcode::RIY_SBC:
      co_yield{ cpu.pc };
      tl = co_yield{ ea++ };
      th = co_yield{ ea };
      ea = t;
      eal += cpu.y;
      if ( eah != th )
      {
        co_yield{ ea };
      }
      d = co_yield{ t };
      if ( cpu.executeR( opint.op, d ) )
      {
        co_yield{ t };
      }
      break;
    case Opcode::WIY_STA:
      co_yield{ cpu.pc };
      tl = co_yield{ ea++ };
      th = co_yield{ ea };
      ea = t;
      eal += cpu.y;
      co_yield{ ea };
      co_yield{ t, cpu.a };
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
      eah = co_yield( cpu.pc++ );
      d = co_yield{ ea };
      cpu.executeR( opint.op, d );
      break;
    case Opcode::RAB_ADC:
    case Opcode::RAB_SBC:
      eah = co_yield( cpu.pc++ );
      d = co_yield{ ea };
      if ( cpu.executeR( opint.op, d ) )
      {
        co_yield{ ea };
      }
      break;
    case Opcode::WAB_STA:
      eah = co_yield( cpu.pc++ );
      co_yield{ ea, cpu.a };
      break;
    case Opcode::WAB_STX:
      eah = co_yield( cpu.pc++ );
      co_yield{ ea, cpu.x };
      break;
    case Opcode::WAB_STY:
      eah = co_yield( cpu.pc++ );
      co_yield{ ea, cpu.y };
      break;
    case Opcode::WAB_STZ:
      eah = co_yield( cpu.pc++ );
      co_yield{ ea, 0x00 };
      break;
      break;
    case Opcode::MAB_ASL:
      eah = co_yield{ cpu.pc++ };
      d = co_yield{ ea };
      cpu.asl( d );
      co_yield{ ea, d };
      break;
    case Opcode::MAB_DEC:
      eah = co_yield{ cpu.pc++ };
      d = co_yield{ ea };
      cpu.setnz( --d );
      co_yield{ ea, d };
      break;
    case Opcode::MAB_INC:
      eah = co_yield{ cpu.pc++ };
      d = co_yield{ ea };
      cpu.setnz( ++d );
      co_yield{ ea, d };
      break;
    case Opcode::MAB_LSR:
      eah = co_yield{ cpu.pc++ };
      d = co_yield{ ea };
      cpu.lsr( d );
      co_yield{ ea, d };
      break;
    case Opcode::MAB_ROL:
      eah = co_yield{ cpu.pc++ };
      d = co_yield{ ea };
      cpu.rol( d );
      co_yield{ ea, d };
      break;
    case Opcode::MAB_ROR:
      eah = co_yield{ cpu.pc++ };
      d = co_yield{ ea };
      cpu.ror( --d );
      co_yield{ ea, d };
      break;
    case Opcode::MAB_TRB:
      eah = co_yield{ cpu.pc++ };
      d = co_yield{ ea };
      cpu.setz( d &= ~cpu.a );
      co_yield{ ea, d };
      break;
    case Opcode::MAB_TSB:
      eah = co_yield{ cpu.pc++ };
      d = co_yield{ ea };
      cpu.setz( d |= cpu.a );
      co_yield{ ea, d };
      break;
    case Opcode::RAX_AND:
    case Opcode::RAX_CMP:
    case Opcode::RAX_EOR:
    case Opcode::RAX_LDA:
    case Opcode::RAX_LDY:
    case Opcode::RAX_ORA:
      eah = co_yield( cpu.pc++ );
      t = ea;
      tl += cpu.x;
      if ( th != eah )
      {
        co_yield{ t };
        ++th;
      }
      d = co_yield{ t };
      cpu.executeR( opint.op, d );
      break;
    case Opcode::RAX_ADC:
    case Opcode::RAX_SBC:
      eah = co_yield( cpu.pc++ );
      t = ea;
      tl += cpu.x;
      if ( th != eah )
      {
        co_yield{ t };
        ++th;
      }
      d = co_yield{ t };
      if ( cpu.executeR( opint.op, d ) )
      {
        co_yield{ t };
      }
      break;
    case Opcode::RAY_AND:
    case Opcode::RAY_CMP:
    case Opcode::RAY_EOR:
    case Opcode::RAY_LDA:
    case Opcode::RAY_LDX:
    case Opcode::RAY_ORA:
      eah = co_yield( cpu.pc++ );
      t = ea;
      tl += cpu.y;
      if ( th != eah )
      {
        co_yield{ t };
        ++th;
      }
      d = co_yield{ t };
      cpu.executeR( opint.op, d );
      break;
      break;
    case Opcode::RAY_ADC:
    case Opcode::RAY_SBC:
      eah = co_yield( cpu.pc++ );
      t = ea;
      tl += cpu.y;
      if ( th != eah )
      {
        co_yield{ t };
        ++th;
      }
      d = co_yield{ t };
      if ( cpu.executeR( opint.op, d ) )
      {
        co_yield{ t };
      }
      break;
    case Opcode::WAX_STA:
      eah = co_yield( cpu.pc++ );
      t = ea;
      ea += cpu.x;
      tl += cpu.x;
      co_yield{ t };
      co_yield{ ea, cpu.a };
      break;
    case Opcode::WAX_STZ:
      eah = co_yield( cpu.pc++ );
      t = ea;
      ea += cpu.x;
      tl += cpu.x;
      co_yield{ t };
      co_yield{ ea, 0x00 };
      break;
    case Opcode::WAY_STA:
      eah = co_yield( cpu.pc++ );
      t = ea;
      ea += cpu.y;
      tl += cpu.y;
      co_yield{ t };
      co_yield{ ea, cpu.a };
      break;
    case Opcode::MAX_ASL:
      eah = co_yield( cpu.pc++ );
      t = ea;
      ea += cpu.x;
      tl += cpu.x;
      co_yield{ t };
      d = co_yield{ ea };
      cpu.asl( d );
      co_yield{ ea, d };
      break;
    case Opcode::MAX_DEC:
      eah = co_yield( cpu.pc++ );
      t = ea;
      ea += cpu.x;
      tl += cpu.x;
      co_yield{ t };
      d = co_yield{ ea };
      cpu.setnz( --d );
      co_yield{ ea, d };
      break;
    case Opcode::MAX_INC:
      eah = co_yield( cpu.pc++ );
      t = ea;
      ea += cpu.x;
      tl += cpu.x;
      co_yield{ t };
      d = co_yield{ ea };
      cpu.setnz( ++d );
      co_yield{ ea, d };
      break;
    case Opcode::MAX_LSR:
      eah = co_yield( cpu.pc++ );
      t = ea;
      ea += cpu.x;
      tl += cpu.x;
      co_yield{ t };
      d = co_yield{ ea };
      cpu.lsr( d );
      co_yield{ ea, d };
      break;
    case Opcode::MAX_ROL:
      eah = co_yield( cpu.pc++ );
      t = ea;
      ea += cpu.x;
      tl += cpu.x;
      co_yield{ t };
      d = co_yield{ ea };
      cpu.rol( d );
      co_yield{ ea, d };
      break;
    case Opcode::MAX_ROR:
      eah = co_yield( cpu.pc++ );
      t = ea;
      ea += cpu.x;
      tl += cpu.x;
      co_yield{ t };
      d = co_yield{ ea };
      cpu.ror( d );
      co_yield{ ea, d };
      break;
    case Opcode::JMA_JMP:
      eah = co_yield( cpu.pc++ );
      cpu.pc = ea;
      break;
    case Opcode::JSA_JSR:
      co_yield{ cpu.s };
      co_yield{ cpu.s--, cpu.pch };
      co_yield{ cpu.s--, cpu.pcl };
      eah = co_yield( cpu.pc++ );
      cpu.pc = ea;
      break;
    case Opcode::JMX_JMP:
      eah = co_yield( cpu.pc++ );
      co_yield{ cpu.pc };
      t = ea;
      eal += cpu.x;
      co_yield{ ea };
      t += cpu.x;
      eal = co_yield{ t++ };
      eah = co_yield{ t };
      cpu.pc = ea;
      break;
    case Opcode::JMI_JMP:
      eah = co_yield( cpu.pc++ );
      tl = co_yield{ ea };
      eal++;
      co_yield{ ea };
      eah += eal == 0 ? 1 : 0;
      th = co_yield{ ea };
      cpu.pc = t;
      break;
    case Opcode::IMP_ASL:
      cpu.asl( cpu.a );
      break;
    case Opcode::IMP_CLC:
      cpu.C = 0;
      break;
    case Opcode::IMP_CLD:
      cpu.D = 0;
      break;
    case Opcode::IMP_CLI:
      cpu.I = 0;
      break;
    case Opcode::IMP_CLV:
      cpu.V = 0;
      break;
    case Opcode::IMP_DEC:
      cpu.setnz( --cpu.a );
      break;
    case Opcode::IMP_DEX:
      cpu.setnz( --cpu.x );
      break;
    case Opcode::IMP_DEY:
      cpu.setnz( --cpu.y );
      break;
    case Opcode::IMP_INC:
      cpu.setnz( ++cpu.a );
      break;
    case Opcode::IMP_INX:
      cpu.setnz( --cpu.x );
      break;
    case Opcode::IMP_INY:
      cpu.setnz( --cpu.y );
      break;
    case Opcode::IMP_LSR:
      cpu.lsr( cpu.a );
      break;
    case Opcode::IMP_NOP:
      break;
    case Opcode::IMP_ROL:
      cpu.rol( cpu.a );
      break;
    case Opcode::IMP_ROR:
      cpu.ror( cpu.a );
      break;
    case Opcode::IMP_SEC:
      cpu.C = 1;
      break;
    case Opcode::IMP_SED:
      cpu.D = 1;
      break;
    case Opcode::IMP_SEI:
      cpu.I = 1;
      break;
    case Opcode::IMP_TAX:
      cpu.setnz( cpu.x = cpu.a );
      break;
    case Opcode::IMP_TAY:
      cpu.setnz( cpu.y = cpu.a );
      break;
    case Opcode::IMP_TSX:
      cpu.setnz( cpu.x = cpu.sl );
      break;
    case Opcode::IMP_TXA:
      cpu.setnz( cpu.a = cpu.x );
      break;
    case Opcode::IMP_TXS:
      cpu.sl = cpu.x;
      break;
    case Opcode::IMP_TYA:
      cpu.setnz( cpu.a = cpu.y );
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
      cpu.executeR( opint.op, eal );
      break;
    case Opcode::IMM_ADC:
    case Opcode::IMM_SBC:
      if ( cpu.executeR( opint.op, eal ) )
      {
        co_yield{ cpu.pc };
      }
      break;
    case Opcode::BRL_BCC:
      if ( !cpu.C )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BRL_BCS:
      if ( cpu.C )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BRL_BEQ:
      if ( cpu.Z )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BRL_BMI:
      if ( cpu.N )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BRL_BNE:
      if ( !cpu.Z )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BRL_BPL:
      if ( !cpu.N )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BRL_BRA:
      co_yield{ cpu.pc };
      t = cpu.pc + (int8_t)eal;
      if ( th != cpu.pch )
      {
        co_yield{ cpu.pc };
      }
      cpu.pc = t;
      break;
    case Opcode::BRL_BVC:
      if ( !cpu.V )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BRL_BVS:
      if ( cpu.V )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BZR_BBR0:
      t = co_yield{ ea };
      if ( ( t & 0x01 ) == 0 )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BZR_BBR1:
      t = co_yield{ ea };
      if ( ( t & 0x02 ) == 0 )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BZR_BBR2:
      t = co_yield{ ea };
      if ( ( t & 0x04 ) == 0 )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BZR_BBR3:
      t = co_yield{ ea };
      if ( ( t & 0x08 ) == 0 )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BZR_BBR4:
      t = co_yield{ ea };
      if ( ( t & 0x10 ) == 0 )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BZR_BBR5:
      t = co_yield{ ea };
      if ( ( t & 0x20 ) == 0 )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BZR_BBR6:
      t = co_yield{ ea };
      if ( ( t & 0x40 ) == 0 )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BZR_BBR7:
      t = co_yield{ ea };
      if ( ( t & 0x80 ) == 0 )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BZR_BBS0:
      t = co_yield{ ea };
      if ( ( t & 0x01 ) == 1 )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BZR_BBS1:
      t = co_yield{ ea };
      if ( ( t & 0x02 ) == 1 )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BZR_BBS2:
      t = co_yield{ ea };
      if ( ( t & 0x04 ) == 1 )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BZR_BBS3:
      t = co_yield{ ea };
      if ( ( t & 0x08 ) == 1 )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BZR_BBS4:
      t = co_yield{ ea };
      if ( ( t & 0x10 ) == 1 )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BZR_BBS5:
      t = co_yield{ ea };
      if ( ( t & 0x20 ) == 1 )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BZR_BBS6:
      t = co_yield{ ea };
      if ( ( t & 0x40 ) == 1 )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BZR_BBS7:
      t = co_yield{ ea };
      if ( ( t & 0x80 ) == 1 )
      {
        co_yield{ cpu.pc };
        t = cpu.pc + (int8_t)eal;
        if ( th != cpu.pch )
        {
          co_yield{ cpu.pc };
        }
        cpu.pc = t;
      }
      break;
    case Opcode::BRK_BRK:
      if ( opint.interrupt & CPU::I_RESET )
      {
        co_yield{ cpu.s-- };
        co_yield{ cpu.s-- };
        co_yield{ cpu.s-- };
        eal = co_yield{ 0xfffc };
        eah = co_yield{ 0xfffd };
        opint.interrupt &= ~CPU::I_RESET;
      }
      else
      {
        co_yield{ cpu.s--, cpu.pch };
        co_yield{ cpu.s--, cpu.pcl };
        if ( opint.interrupt == CPU::I_NONE )
        {
          co_yield{ cpu.s--, cpu.pbrk() };
        }
        else
        {
          co_yield{ cpu.s--, cpu.p() };
        }
        if ( opint.interrupt & CPU::I_NMI )
        {
          eal = co_yield{ 0xfffa };
          eah = co_yield{ 0xfffb };
          opint.interrupt &= ~CPU::I_NMI;
        }
        else
        {
          eal = co_yield{ 0xfffe };
          eah = co_yield{ 0xffff };
          if ( opint.interrupt == CPU::I_IRQ )
          {
            opint.interrupt &= ~CPU::I_IRQ;
          }
        }
      }
      cpu.pc = ea;
     break;
    case Opcode::RTI_RTI:
      cpu.P = co_yield{ ++cpu.s };
      eal = co_yield{ ++cpu.s };
      eah = co_yield{ ++cpu.s };
      co_yield{ cpu.pc };
      cpu.pc = ea;
      break;
    case Opcode::RTS_RTS:
      co_yield{ cpu.pc };
      eal = co_yield{ ++cpu.s };
      eah = co_yield{ ++cpu.s };
      co_yield{ cpu.pc };
      cpu.pc = ea;
      break;
    case Opcode::PHR_PHA:
      co_yield{ cpu.s--, cpu.a };
      break;
    case Opcode::PHR_PHP:
      co_yield{ cpu.s--, cpu.p() };
      break;
    case Opcode::PHR_PHX:
      co_yield{ cpu.s--, cpu.x };
      break;
    case Opcode::PHR_PHY:
      co_yield{ cpu.s--, cpu.y };
      break;
    case Opcode::PLR_PLA:
      co_yield{ cpu.pc };
      cpu.a = co_yield{ ++cpu.s };
      break;
    case Opcode::PLR_PLP:
      co_yield{ cpu.pc };
      cpu.P = co_yield{ ++cpu.s };
      break;
    case Opcode::PLR_PLX:
      co_yield{ cpu.pc };
      cpu.x = co_yield{ ++cpu.s };
      break;
    case Opcode::PLR_PLY:
      co_yield{ cpu.pc };
      cpu.y = co_yield{ ++cpu.s };
      break;
    case Opcode::UND_2_02:
    case Opcode::UND_2_22:
    case Opcode::UND_2_42:
    case Opcode::UND_2_62:
    case Opcode::UND_2_82:
    case Opcode::UND_2_C2:
    case Opcode::UND_2_E2:
      break;
    case Opcode::UND_3_44:
      co_yield{ ea };
      break;
    case Opcode::UND_4_54:
    case Opcode::UND_4_d4:
    case Opcode::UND_4_f4:
    case Opcode::UND_4_dc:
    case Opcode::UND_4_fc:
      co_yield{ cpu.pc };
      eal += cpu.x;
      eal = co_yield{ ea };
      break;
    case Opcode::UND_8_5c:
      //http://laughtonelectronics.com/Arcana/KimKlone/Kimklone_opcode_mapping.html
      //op - code 5C consumes 3 bytes and 8 cycles but conforms to no known address mode; it remains interesting but useless.
      //I tested the instruction "5C 1234h" ( stored little - endian as 5Ch 34h 12h ) as an example, and observed the following : 3 cycles fetching the instruction, 1 cycle reading FF34, then 4 cycles reading FFFF.
      co_yield{ cpu.pc++ };
      eah = 0xff;
      co_yield{ ea };
      co_yield{ 0xffff };
      co_yield{ 0xffff };
      co_yield{ 0xffff };
      co_yield{ 0xffff };
      break;
    }

    for ( ;; )
    {
      opint = co_yield{ cpu.pc, Opcode{} };
      if ( isHiccup( opint.op ) )
        continue;
      else
        break;
    }
  }

}


AwaitRead CpuLoop::promise_type::yield_value( Read r )
{
  return AwaitRead{ mBus->request( r ) };
}

AwaitReadOpcode CpuLoop::promise_type::yield_value( ReadOpcode r )
{
  return AwaitReadOpcode{ mBus->request( r ) };
}

AwaitWrite CpuLoop::promise_type::yield_value( Write w )
{
  return AwaitWrite{ mBus->request( w ) };
}

void CPU::asl( uint8_t & val )
{
  C = val >> 7;
  setnz( val <<= 1 );
}

void CPU::lsr( uint8_t & val )
{
  C = val & 0x01;
  setnz( val >>= 1 );
}

void CPU::rol( uint8_t & val )
{
  C = val >> 7;
  val <<= 1;
  setnz( val |= C );
}

void CPU::ror( uint8_t & val )
{
  C = val & 1;
  val >>= 1;
  setnz( val |= ( C << 7 ) );
}

bool CPU::executeR( Opcode opcode, uint8_t value )
{
  switch ( opcode )
  {
  case Opcode::RZP_ADC:
    if ( D )
    {
      int lo = ( a & 0x0f ) + ( value & 0x0f ) + C;
      int hi = ( a & 0xf0 ) + ( value & 0xf0 );
      V = 0;
      C = 0;
      if ( lo > 0x09 )
      {
        hi += 0x10;
        lo += 0x06;
      }
      if ( ~( a ^ value ) & ( a ^ hi ) & 0x80 )
      {
        V = 1;
      }
      if ( hi > 0x90 )
      {
        hi += 0x60;
      }
      if ( hi & 0xff00 )
      {
        C = 1;
      }
      a = ( lo & 0x0f ) + ( hi & 0xf0 );
      setnz( a );
      return true;
    }
    else
    {
      int sum = a + value + C;
      V = 0;
      C = 0;
      if ( ~( a ^ value ) & ( a ^ sum ) & 0x80 )
      {
        V = 1;
      }
      if ( sum & 0xff00 )
      {
        C = 1;
      }
      a = (uint8_t)sum;
      setnz( a );
    }
    break;
  case Opcode::RZP_AND:
    setnz( a &= value );
    break;
  case Opcode::RZP_BIT:
    setnz( value );
    V = ( value & 0x40 ) == 0x00 ? 0 : 1;
    break;
  case Opcode::RZP_CMP:
    C = 0;
    if ( a >= value ) C = 1;
    setnz( a - value );
    break;
  case Opcode::RZP_CPX:
    C = 0;
    if ( x >= value ) C = 1;
    setnz( x - value );
    break;
  case Opcode::RZP_CPY:
    C = 0;
    if ( y >= value ) C = 1;
    setnz( y - value );
    break;
  case Opcode::RZP_EOR:
    setnz( a ^= value );
    break;
  case Opcode::RZP_LDA:
    setnz( a = value );
    break;
  case Opcode::RZP_LDX:
    setnz( x |= value );
    break;
  case Opcode::RZP_LDY:
    setnz( y |= value );
    break;
  case Opcode::RZP_ORA:
    setnz( a |= value );
    break;
  case Opcode::RZP_SBC:
    if ( D )
    {
      int sum = a - value - C;
      int lo = ( a & 0x0f ) - ( value & 0x0f ) - C;
      int hi = ( a & 0xf0 ) - ( value & 0xf0 );
      V = 0;
      C = 0;
      if ( ( a ^ value ) & ( a ^ sum ) & 0x80 )
      {
        V = 1;
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
        C = 1;
      }
      a = ( lo & 0x0f ) + ( hi & 0xf0 );
      setnz( a );
      return true;
    }
    else
    {
      int sum = a - value - C;
      V = 0;
      C = 0;
      if ( ( a ^ value ) & ( a ^ sum ) & 0x80 )
      {
        V = 1;
      }
      if ( ( sum & 0xff00 ) == 0 )
      {
        C = 1;
      }
      a = (uint8_t)sum;
      setnz( a );
    }
  default:
    break;
  }

  return false;
}


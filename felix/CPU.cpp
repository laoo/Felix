#include "CPU.hpp"
#include "Opcodes.hpp"
#include <cassert>

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

CpuExecute CPU::execute( Felix & felix )
{
  co_await felix;

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

    if ( interrupt && ( !get<bitI>() || ( interrupt & ~CPU::I_IRQ ) != 0 ) )
    {
      opcode = Opcode::BRK_BRK;
    }

    operand = eal = co_await CPUFetchOperand{ pc };

    switch ( opcode )
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
      ++pc;
      d = co_await CPURead{ ea };
      executeCommon( opcode, d );
      break;
    case Opcode::RZP_ADC:
    case Opcode::RZP_SBC:
      ++pc;
      d = co_await CPURead{ ea };
      if ( executeCommon( opcode, d ) )
      {
        co_await CPURead{ ea };
      }
      break;
    case Opcode::WZP_STA:
      ++pc;
      co_await CPUWrite{ ea, a };
      break;
    case Opcode::WZP_STX:
      ++pc;
      co_await CPUWrite{ ea, x };
      break;
    case Opcode::WZP_STY:
      ++pc;
      co_await CPUWrite{ ea, y };
      break;
    case Opcode::WZP_STZ:
      ++pc;
      co_await CPUWrite{ ea, 0x00 };
      break;
    case Opcode::MZP_ASL:
      ++pc;
      d = co_await CPURead{ ea };
      asl( d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_DEC:
      ++pc;
      d = co_await CPURead{ ea };
      setnz( --d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_INC:
      ++pc;
      d = co_await CPURead{ ea };
      setnz( ++d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_LSR:
      ++pc;
      d = co_await CPURead{ ea };
      lsr( d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_ROL:
      ++pc;
      d = co_await CPURead{ ea };
      rol( d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_ROR:
      ++pc;
      ror( d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_TRB:
      ++pc;
      d = co_await CPURead{ ea };
      setz( d & a );
      d &= ~a;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_TSB:
      ++pc;
      d = co_await CPURead{ ea };
      setz( d & a );
      d |= a;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_RMB0:
      ++pc;
      d = co_await CPURead{ ea };
      d &= ~0x01;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_RMB1:
      ++pc;
      d = co_await CPURead{ ea };
      d &= ~0x02;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_RMB2:
      ++pc;
      d = co_await CPURead{ ea };
      d &= ~0x04;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_RMB3:
      ++pc;
      d = co_await CPURead{ ea };
      d &= ~0x08;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_RMB4:
      ++pc;
      d = co_await CPURead{ ea };
      d &= ~0x10;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_RMB5:
      ++pc;
      d = co_await CPURead{ ea };
      d &= ~0x20;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_RMB6:
      ++pc;
      d = co_await CPURead{ ea };
      d &= ~0x40;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_RMB7:
      ++pc;
      d = co_await CPURead{ ea };
      d &= ~0x80;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_SMB0:
      ++pc;
      d = co_await CPURead{ ea };
      d |= 0x01;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_SMB1:
      ++pc;
      d = co_await CPURead{ ea };
      d |= 0x02;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_SMB2:
      ++pc;
      d = co_await CPURead{ ea };
      d |= 0x04;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_SMB3:
      ++pc;
      d = co_await CPURead{ ea };
      d |= 0x08;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_SMB4:
      ++pc;
      d = co_await CPURead{ ea };
      d |= 0x10;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_SMB5:
      ++pc;
      d = co_await CPURead{ ea };
      d |= 0x20;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_SMB6:
      ++pc;
      d = co_await CPURead{ ea };
      d |= 0x40;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZP_SMB7:
      ++pc;
      d = co_await CPURead{ ea };
      d |= 0x80;
      co_await CPUWrite{ ea, d };
      break;
      break;
    case Opcode::RZX_AND:
    case Opcode::RZX_BIT:
    case Opcode::RZX_CMP:
    case Opcode::RZX_EOR:
    case Opcode::RZX_LDA:
    case Opcode::RZX_LDY:
    case Opcode::RZX_ORA:
      co_await CPURead{ ++pc };
      eal += x;
      d = co_await CPURead{ ea };
      executeCommon( opcode, d );
      break;
    case Opcode::RZX_ADC:
    case Opcode::RZX_SBC:
      co_await CPURead{ ++pc };
      eal += x;
      d = co_await CPURead{ ea };
      if ( executeCommon( opcode, d ) )
      {
        co_await CPURead{ ea };
      }
      break;
    case Opcode::RZY_LDX:
      co_await CPURead{ ++pc };
      eal += y;
      setnz( x = co_await CPURead{ ea } );
      break;
    case Opcode::WZX_STA:
      co_await CPURead{ ++pc };
      eal += x;
      co_await CPUWrite{ ea, a };
      break;
    case Opcode::WZX_STY:
      co_await CPURead{ ++pc };
      eal += x;
      co_await CPUWrite{ ea, y };
      break;
    case Opcode::WZX_STZ:
      co_await CPURead{ ++pc };
      eal += x;
      co_await CPUWrite{ ea, 0x00 };
      break;
    case Opcode::WZY_STX:
      co_await CPURead{ ++pc };
      eal += y;
      co_await CPUWrite{ ea, x };
      break;
    case Opcode::MZX_ASL:
      co_await CPURead{ ++pc };
      eal += x;
      d = co_await CPURead{ ea };
      asl( d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZX_DEC:
      co_await CPURead{ ++pc };
      eal += x;
      d = co_await CPURead{ ea };
      setnz( --d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZX_INC:
      co_await CPURead{ ++pc };
      eal += x;
      d = co_await CPURead{ ea };
      setnz( ++d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZX_LSR:
      co_await CPURead{ ++pc };
      eal += x;
      d = co_await CPURead{ ea };
      lsr( d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZX_ROL:
      co_await CPURead{ ++pc };
      eal += x;
      d = co_await CPURead{ ea };
      rol( d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MZX_ROR:
      co_await CPURead{ ++pc };
      eal += x;
      d = co_await CPURead{ ea };
      ror( d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::RIN_AND:
    case Opcode::RIN_CMP:
    case Opcode::RIN_EOR:
    case Opcode::RIN_LDA:
    case Opcode::RIN_ORA:
      ++pc;
      tl = co_await CPURead{ ea++ };
      th = co_await CPURead{ ea };
      d = co_await CPURead{ t };
      executeCommon( opcode, d );
      break;
    case Opcode::RIN_ADC:
    case Opcode::RIN_SBC:
      ++pc;
      tl = co_await CPURead{ ea++ };
      th = co_await CPURead{ ea };
      d = co_await CPURead{ t };
      if ( executeCommon( opcode, d ) )
      {
        co_await CPURead{ t };
      }
      break;
    case Opcode::WIN_STA:
      ++pc;
      tl = co_await CPURead{ ea++ };
      th = co_await CPURead{ ea };
      co_await CPUWrite{ t, a };
      break;
    case Opcode::RIX_AND:
    case Opcode::RIX_CMP:
    case Opcode::RIX_EOR:
    case Opcode::RIX_LDA:
    case Opcode::RIX_ORA:
      co_await CPURead{ ++pc };
      eal += x;
      tl = co_await CPURead{ ea++ };
      th = co_await CPURead{ ea };
      d = co_await CPURead{ t };
      executeCommon( opcode, d );
      break;
    case Opcode::RIX_ADC:
    case Opcode::RIX_SBC:
      co_await CPURead{ ++pc };
      eal += x;
      tl = co_await CPURead{ ea++ };
      th = co_await CPURead{ ea };
      d = co_await CPURead{ t };
      if ( executeCommon( opcode, d ) )
      {
        co_await CPURead{ t };
      }
      break;
    case Opcode::WIX_STA:
      co_await CPURead{ ++pc };
      eal += x;
      tl = co_await CPURead{ ea++ };
      th = co_await CPURead{ ea };
      co_await CPUWrite{ t, a };
      break;
    case Opcode::RIY_AND:
    case Opcode::RIY_CMP:
    case Opcode::RIY_EOR:
    case Opcode::RIY_LDA:
    case Opcode::RIY_ORA:
      co_await CPURead{ ++pc };
      tl = co_await CPURead{ ea++ };
      th = co_await CPURead{ ea };
      ea = t;
      ea += y;
      if ( eah != th )
      {
        tl += y;
        co_await CPURead{ t };
      }
      d = co_await CPURead{ ea };
      executeCommon( opcode, d );
      break;
    case Opcode::RIY_ADC:
    case Opcode::RIY_SBC:
      co_await CPURead{ ++pc };
      tl = co_await CPURead{ ea++ };
      th = co_await CPURead{ ea };
      ea = t;
      ea += y;
      if ( eah != th )
      {
        tl += y;
        co_await CPURead{ t };
      }
      d = co_await CPURead{ ea };
      if ( executeCommon( opcode, d ) )
      {
        co_await CPURead{ t };
      }
      break;
    case Opcode::WIY_STA:
      co_await CPURead{ ++pc };
      tl = co_await CPURead{ ea++ };
      th = co_await CPURead{ ea };
      ea = t;
      ea += y;
      tl += y;
      co_await CPURead{ t };
      co_await CPUWrite{ ea, a };
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
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      d = co_await CPURead{ ea };
      executeCommon( opcode, d );
      break;
    case Opcode::RAB_ADC:
    case Opcode::RAB_SBC:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      d = co_await CPURead{ ea };
      if ( executeCommon( opcode, d ) )
      {
        co_await CPURead{ ea };
      }
      break;
    case Opcode::WAB_STA:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      co_await CPUWrite{ ea, a };
      break;
    case Opcode::WAB_STX:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      co_await CPUWrite{ ea, x };
      break;
    case Opcode::WAB_STY:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      co_await CPUWrite{ ea, y };
      break;
    case Opcode::WAB_STZ:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      co_await CPUWrite{ ea, 0x00 };
      break;
      break;
    case Opcode::MAB_ASL:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      d = co_await CPURead{ ea };
      asl( d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MAB_DEC:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      d = co_await CPURead{ ea };
      setnz( --d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MAB_INC:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      d = co_await CPURead{ ea };
      setnz( ++d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MAB_LSR:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      d = co_await CPURead{ ea };
      lsr( d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MAB_ROL:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      d = co_await CPURead{ ea };
      rol( d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MAB_ROR:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      d = co_await CPURead{ ea };
      ror( d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MAB_TRB:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      d = co_await CPURead{ ea };
      setz( d & a );
      d &= ~a;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MAB_TSB:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      d = co_await CPURead{ ea };
      setz( d & a );
      d |= a;
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::RAX_AND:
    case Opcode::RAX_BIT:
    case Opcode::RAX_CMP:
    case Opcode::RAX_EOR:
    case Opcode::RAX_LDA:
    case Opcode::RAX_LDY:
    case Opcode::RAX_ORA:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      t = ea;
      t += x;
      if ( th != eah )
      {
        eal += x;
        co_await CPURead{ ea };
      }
      d = co_await CPURead{ t };
      executeCommon( opcode, d );
      break;
    case Opcode::RAX_ADC:
    case Opcode::RAX_SBC:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      t = ea;
      t += x;
      if ( th != eah )
      {
        eal += x;
        co_await CPURead{ ea };
      }
      d = co_await CPURead{ t };
      if ( executeCommon( opcode, d ) )
      {
        co_await CPURead{ t };
      }
      break;
    case Opcode::RAY_AND:
    case Opcode::RAY_CMP:
    case Opcode::RAY_EOR:
    case Opcode::RAY_LDA:
    case Opcode::RAY_LDX:
    case Opcode::RAY_ORA:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      t = ea;
      t += y;
      if ( th != eah )
      {
        eal += x;
        co_await CPURead{ ea };
      }
      d = co_await CPURead{ t };
      executeCommon( opcode, d );
      break;
    case Opcode::RAY_ADC:
    case Opcode::RAY_SBC:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      t = ea;
      t += y;
      if ( th != eah )
      {
        eal += x;
        co_await CPURead{ ea };
      }
      d = co_await CPURead{ t };
      if ( executeCommon( opcode, d ) )
      {
        co_await CPURead{ t };
      }
      break;
    case Opcode::WAX_STA:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      t = ea;
      eal += x;
      t += x;
      co_await CPURead{ ea };
      co_await CPUWrite{ t, a };
      break;
    case Opcode::WAX_STZ:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      t = ea;
      eal += x;
      t += x;
      co_await CPURead{ ea };
      co_await CPUWrite{ t, 0x00 };
      break;
    case Opcode::WAY_STA:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      t = ea;
      eal += y;
      t += y;
      co_await CPURead{ ea };
      co_await CPUWrite{ t, a };
      break;
    case Opcode::MAX_ASL:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      t = ea;
      eal += x;
      t += x;
      co_await CPURead{ ea };
      d = co_await CPURead{ t };
      asl( d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MAX_DEC:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      t = ea;
      eal += x;
      t += x;
      co_await CPURead{ ea };
      d = co_await CPURead{ t };
      setnz( --d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MAX_INC:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      t = ea;
      eal += x;
      t += x;
      co_await CPURead{ ea };
      d = co_await CPURead{ t };
      setnz( ++d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MAX_LSR:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      t = ea;
      eal += x;
      t += x;
      co_await CPURead{ ea };
      d = co_await CPURead{ t };
      lsr( d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MAX_ROL:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      t = ea;
      eal += x;
      t += x;
      co_await CPURead{ ea };
      d = co_await CPURead{ t };
      rol( d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::MAX_ROR:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      t = ea;
      eal += x;
      t += x;
      co_await CPURead{ ea };
      d = co_await CPURead{ t };
      ror( d );
      co_await CPUWrite{ ea, d };
      break;
    case Opcode::JMA_JMP:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      pc = ea;
      break;
    case Opcode::JSA_JSR:
      ++pc;
      co_await CPURead{ s };
      co_await CPUWrite{ s, pch };
      sl--;
      co_await CPUWrite{ s, pcl };
      sl--;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      pc = ea;
      break;
    case Opcode::JMX_JMP:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      co_await CPURead{ pc };
      t = ea;
      eal += x;
      co_await CPURead{ ea };
      t += x;
      eal = co_await CPURead{ t++ };
      eah = co_await CPURead{ t };
      pc = ea;
      break;
    case Opcode::JMI_JMP:
      ++pc;
      operand = eah = co_await CPUFetchOperand{ pc++ };
      tl = co_await CPURead{ ea };
      eal++;
      co_await CPURead{ ea };
      eah += eal == 0 ? 1 : 0;
      th = co_await CPURead{ ea };
      pc = t;
      break;
    case Opcode::IMP_ASL:
      asl( a );
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
      setnz( --a );
      break;
    case Opcode::IMP_DEX:
      setnz( --x );
      break;
    case Opcode::IMP_DEY:
      setnz( --y );
      break;
    case Opcode::IMP_INC:
      setnz( ++a );
      break;
    case Opcode::IMP_INX:
      setnz( ++x );
      break;
    case Opcode::IMP_INY:
      setnz( ++y );
      break;
    case Opcode::IMP_LSR:
      lsr( a );
      break;
    case Opcode::IMP_NOP:
      break;
    case Opcode::IMP_ROL:
      rol( a );
      break;
    case Opcode::IMP_ROR:
      ror( a );
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
      setnz( x = a );
      break;
    case Opcode::IMP_TAY:
      setnz( y = a );
      break;
    case Opcode::IMP_TSX:
      setnz( x = sl );
      break;
    case Opcode::IMP_TXA:
      setnz( a = x );
      break;
    case Opcode::IMP_TXS:
      sl = x;
      break;
    case Opcode::IMP_TYA:
      setnz( a = y );
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
      ++pc;
      executeCommon( opcode, eal );
      break;
    case Opcode::IMM_ADC:
    case Opcode::IMM_SBC:
      ++pc;
      if ( executeCommon( opcode, eal ) )
      {
        co_await CPURead{ pc };
      }
      break;
    case Opcode::BRL_BCC:
      ++pc;
      if ( !get<bitC>() )
      {
        co_await CPURead{ pc };
        t = pc + ( int8_t )eal;
        if ( th != pch )
        {
          co_await CPURead{ pc };
        }
        pc = t;
      }
      else
      {
        operand -= 1;
      }
      break;
    case Opcode::BRL_BCS:
      ++pc;
      if ( get<bitC>() )
      {
        co_await CPURead{ pc };
        t = pc + ( int8_t )eal;
        if ( th != pch )
        {
          co_await CPURead{ pc };
        }
        pc = t;
      }
      else
      {
        operand -= 1;
      }
      break;
    case Opcode::BRL_BEQ:
      ++pc;
      if ( get<bitZ>() )
      {
        co_await CPURead{ pc };
        t = pc + ( int8_t )eal;
        if ( th != pch )
        {
          co_await CPURead{ pc };
        }
        pc = t;
      }
      else
      {
        operand -= 1;
      }
      break;
    case Opcode::BRL_BMI:
      ++pc;
      if ( get<bitN>() )
      {
        co_await CPURead{ pc };
        t = pc + ( int8_t )eal;
        if ( th != pch )
        {
          co_await CPURead{ pc };
        }
        pc = t;
      }
      else
      {
        operand -= 1;
      }
      break;
    case Opcode::BRL_BNE:
      ++pc;
      if ( !get<bitZ>() )
      {
        co_await CPURead{ pc };
        t = pc + ( int8_t )eal;
        if ( th != pch )
        {
          co_await CPURead{ pc };
        }
        pc = t;
      }
      else
      {
        operand -= 1;
      }
      break;
    case Opcode::BRL_BPL:
      ++pc;
      if ( !get<bitN>() )
      {
        co_await CPURead{ pc };
        t = pc + ( int8_t )eal;
        if ( th != pch )
        {
          co_await CPURead{ pc };
        }
        pc = t;
      }
      else
      {
        operand -= 1;
      }
      break;
    case Opcode::BRL_BRA:
      operand -= 1;
      co_await CPURead{ ++pc };
      t = pc + ( int8_t )eal;
      if ( th != pch )
      {
        co_await CPURead{ pc };
      }
      pc = t;
      break;
    case Opcode::BRL_BVC:
      ++pc;
      if ( !get<bitV>() )
      {
        co_await CPURead{ pc };
        t = pc + ( int8_t )eal;
        if ( th != pch )
        {
          co_await CPURead{ pc };
        }
        pc = t;
      }
      else
      {
        operand -= 1;
      }
      break;
    case Opcode::BRL_BVS:
      ++pc;
      if ( get<bitV>() )
      {
        co_await CPURead{ pc };
        t = pc + ( int8_t )eal;
        if ( th != pch )
        {
          co_await CPURead{ pc };
        }
        pc = t;
      }
      else
      {
        operand -= 1;
      }
      break;
    case Opcode::BZR_BBR0:
      ++pc;
      eal = co_await CPURead{ ea };
      operand = tl = co_await CPUFetchOperand{ pc++ };
      co_await CPURead{ ea };
      if ( ( eal & 0x01 ) == 0 )
      {
        t = pc + ( int8_t )tl;
        pc = t;
      }
      break;
    case Opcode::BZR_BBR1:
      ++pc;
      eal = co_await CPURead{ ea };
      operand = tl = co_await CPUFetchOperand{ pc++ };
      co_await CPURead{ ea };
      if ( ( eal & 0x02 ) == 0 )
      {
        t = pc + ( int8_t )tl;
        pc = t;
      }
      break;
    case Opcode::BZR_BBR2:
      ++pc;
      eal = co_await CPURead{ ea };
      operand = tl = co_await CPUFetchOperand{ pc++ };
      co_await CPURead{ ea };
      if ( ( eal & 0x04 ) == 0 )
      {
        t = pc + ( int8_t )tl;
        pc = t;
      }
      break;
    case Opcode::BZR_BBR3:
      ++pc;
      eal = co_await CPURead{ ea };
      operand = tl = co_await CPUFetchOperand{ pc++ };
      co_await CPURead{ ea };
      if ( ( eal & 0x08 ) == 0 )
      {
        t = pc + ( int8_t )tl;
        pc = t;
      }
      break;
    case Opcode::BZR_BBR4:
      ++pc;
      eal = co_await CPURead{ ea };
      operand = tl = co_await CPUFetchOperand{ pc++ };
      co_await CPURead{ ea };
      if ( ( eal & 0x10 ) == 0 )
      {
        t = pc + ( int8_t )tl;
        pc = t;
      }
      break;
    case Opcode::BZR_BBR5:
      ++pc;
      eal = co_await CPURead{ ea };
      operand = tl = co_await CPUFetchOperand{ pc++ };
      co_await CPURead{ ea };
      if ( ( eal & 0x20 ) == 0 )
      {
        t = pc + ( int8_t )tl;
        pc = t;
      }
      break;
    case Opcode::BZR_BBR6:
      ++pc;
      eal = co_await CPURead{ ea };
      operand = tl = co_await CPUFetchOperand{ pc++ };
      co_await CPURead{ ea };
      if ( ( eal & 0x40 ) == 0 )
      {
        t = pc + ( int8_t )tl;
        pc = t;
      }
      break;
    case Opcode::BZR_BBR7:
      ++pc;
      eal = co_await CPURead{ ea };
      operand = tl = co_await CPUFetchOperand{ pc++ };
      co_await CPURead{ ea };
      if ( ( eal & 0x80 ) == 0 )
      {
        t = pc + ( int8_t )tl;
        pc = t;
      }
      break;
    case Opcode::BZR_BBS0:
      ++pc;
      eal = co_await CPURead{ ea };
      operand = tl = co_await CPUFetchOperand{ pc++ };
      co_await CPURead{ ea };
      if ( ( eal & 0x01 ) != 0 )
      {
        t = pc + ( int8_t )tl;
        pc = t;
      }
      break;
    case Opcode::BZR_BBS1:
      ++pc;
      eal = co_await CPURead{ ea };
      operand = tl = co_await CPUFetchOperand{ pc++ };
      co_await CPURead{ ea };
      if ( ( eal & 0x02 ) != 0 )
      {
        t = pc + ( int8_t )tl;
        pc = t;
      }
      break;
    case Opcode::BZR_BBS2:
      ++pc;
      eal = co_await CPURead{ ea };
      operand = tl = co_await CPUFetchOperand{ pc++ };
      co_await CPURead{ ea };
      if ( ( eal & 0x04 ) != 0 )
      {
        t = pc + ( int8_t )tl;
        pc = t;
      }
      break;
    case Opcode::BZR_BBS3:
      ++pc;
      eal = co_await CPURead{ ea };
      operand = tl = co_await CPUFetchOperand{ pc++ };
      co_await CPURead{ ea };
      if ( ( eal & 0x08 ) != 0 )
      {
        t = pc + ( int8_t )tl;
        pc = t;
      }
      break;
    case Opcode::BZR_BBS4:
      ++pc;
      eal = co_await CPURead{ ea };
      operand = tl = co_await CPUFetchOperand{ pc++ };
      co_await CPURead{ ea };
      if ( ( eal & 0x10 ) != 0 )
      {
        t = pc + ( int8_t )tl;
        pc = t;
      }
      break;
    case Opcode::BZR_BBS5:
      ++pc;
      eal = co_await CPURead{ ea };
      operand = tl = co_await CPUFetchOperand{ pc++ };
      co_await CPURead{ ea };
      if ( ( eal & 0x20 ) != 0 )
      {
        t = pc + ( int8_t )tl;
        pc = t;
      }
      break;
    case Opcode::BZR_BBS6:
      ++pc;
      eal = co_await CPURead{ ea };
      operand = tl = co_await CPUFetchOperand{ pc++ };
      co_await CPURead{ ea };
      if ( ( eal & 0x40 ) != 0 )
      {
        t = pc + ( int8_t )tl;
        pc = t;
      }
      break;
    case Opcode::BZR_BBS7:
      ++pc;
      eal = co_await CPURead{ ea };
      operand = tl = co_await CPUFetchOperand{ pc++ };
      co_await CPURead{ ea };
      if ( ( eal & 0x80 ) != 0 )
      {
        t = pc + ( int8_t )tl;
        pc = t;
      }
      break;
    case Opcode::BRK_BRK:
      if ( interrupt & CPU::I_RESET )
      {
        co_await CPURead{ s };
        sl--;
        co_await CPURead{ s };
        sl--;
        co_await CPURead{ s };
        sl--;
        eal = co_await CPURead{ 0xfffc };
        eah = co_await CPURead{ 0xfffd };
      }
      else
      {
        //on interrupt PC should point to interrupted instruction
        //on BRK should after past BRK argument
        pc += interrupt ? -1 : 1;
        co_await CPUWrite{ s, pch };
        sl--;
        co_await CPUWrite{ s, pcl };
        sl--;
        if ( interrupt )
        {
          co_await CPUWrite{ s, pirq() };
        }
        else
        {
          co_await CPUWrite{ s, p() };
        }
        sl--;
        if ( interrupt & CPU::I_NMI )
        {
          eal = co_await CPURead{ 0xfffa };
          eah = co_await CPURead{ 0xfffb };
        }
        else
        {
          eal = co_await CPURead{ 0xfffe };
          eah = co_await CPURead{ 0xffff };
        }
        set<bitI>();
      }
      clear<bitD>();
      pc = ea;
      break;
    case Opcode::RTI_RTI:
      ++pc;
      ++sl;
      P = co_await CPURead{ s };
      ++sl;
      eal = co_await CPURead{ s };
      ++sl;
      eah = co_await CPURead{ s };
      co_await CPURead{ pc };
      pc = ea;
      break;
    case Opcode::RTS_RTS:
      co_await CPURead{ ++pc };
      ++sl;
      eal = co_await CPURead{ s };
      ++sl;
      eah = co_await CPURead{ s };
      co_await CPURead{ pc };
      ++ea;
      pc = ea;
      break;
    case Opcode::PHR_PHA:
      co_await CPUWrite{ s, a };
      sl--;
      break;
    case Opcode::PHR_PHP:
      co_await CPUWrite{ s, p() };
      sl--;
      break;
    case Opcode::PHR_PHX:
      co_await CPUWrite{ s, x };
      sl--;
      break;
    case Opcode::PHR_PHY:
      co_await CPUWrite{ s, y };
      sl--;
      break;
    case Opcode::PLR_PLA:
      co_await CPURead{ pc };
      ++sl;
      setnz( a = co_await CPURead{ s } );
      break;
    case Opcode::PLR_PLP:
      co_await CPURead{ pc };
      ++sl;
      P = co_await CPURead{ s };
      break;
    case Opcode::PLR_PLX:
      co_await CPURead{ pc };
      ++sl;
      setnz( x = co_await CPURead{ s } );
      break;
    case Opcode::PLR_PLY:
      co_await CPURead{ pc };
      ++sl;
      setnz( y = co_await CPURead{ s } );
      break;
    case Opcode::UND_2_02:
    case Opcode::UND_2_22:
    case Opcode::UND_2_42:
    case Opcode::UND_2_62:
    case Opcode::UND_2_82:
    case Opcode::UND_2_C2:
    case Opcode::UND_2_E2:
      ++pc;
      break;
    case Opcode::UND_3_44:
      ++pc;
      co_await CPURead{ ea };
      break;
    case Opcode::UND_4_54:
    case Opcode::UND_4_d4:
    case Opcode::UND_4_f4:
      ++pc;
      co_await CPURead{ pc };
      eal += x;
      eal = co_await CPURead{ ea };
      break;
    case Opcode::UND_4_dc:
    case Opcode::UND_4_fc:
      ++pc;
      eah = co_await CPURead{ pc++ };
      co_await CPURead{ ea };
      break;
    case Opcode::UND_8_5c:
      //http://laughtonelectronics.com/Arcana/KimKlone/Kimklone_opcode_mapping.html
      //op - code 5C consumes 3 bytes and 8 cycles but conforms to no known address mode; it remains interesting but useless.
      //I tested the instruction "5C 1234h" ( stored little - endian as 5Ch 34h 12h ) as an example, and observed the following : 3 cycles fetching the instruction, 1 cycle reading FF34, then 4 cycles reading FFFF.
      ++pc;
      co_await CPURead{ pc++ };
      eah = 0xff;
      co_await CPURead{ ea };
      co_await CPURead{ 0xffff };
      co_await CPURead{ 0xffff };
      co_await CPURead{ 0xffff };
      co_await CPURead{ 0xffff };
      break;
    default:  //for UND_1_xx
      break;
    }

    for ( ;; )
    {
      auto opint = co_await CPUFetchOpcode{ pc++ };
      opcode = opint.op;
      interrupt = opint.interrupt;
      tick = opint.tick;
      if ( isHiccup( opcode ) )
        continue;
      else
        break;
    }
  }

}

void CPU::asl( uint8_t & val )
{
  set<bitC>( val >= 0x80 );
  setnz( val <<= 1 );
}

void CPU::lsr( uint8_t & val )
{
  set<bitC>( ( val & 0x01 ) != 0 );
  setnz( val >>= 1 );
}

void CPU::rol( uint8_t & val )
{
  int roled = val << 1;
  val = roled & 0xff | ( get<bitC>() ? 0x01 : 0 );
  setnz( val );
  set<bitC>( ( roled & 0x100 ) != 0 );
}

void CPU::ror( uint8_t & val )
{
  bool newC = ( val & 1 ) != 0;
  val = ( val >> 1 ) | ( get<bitC>() ? 0x80 : 0 );
  setnz( val );
  set<bitC>( newC );
}

bool CPU::executeCommon( Opcode opcode, uint8_t value )
{
  switch ( opcode )
  {
  case Opcode::RZP_ADC:
  case Opcode::RZX_ADC:
  case Opcode::RIN_ADC:
  case Opcode::RIX_ADC:
  case Opcode::RIY_ADC:
  case Opcode::RAB_ADC:
  case Opcode::RAX_ADC:
  case Opcode::RAY_ADC:
  case Opcode::IMM_ADC:
    if ( get<bitD>() )
    {
      int lo = ( a & 0x0f ) + ( value & 0x0f ) + ( get<bitC>() ? 0x01 : 0 );
      int hi = ( a & 0xf0 ) + ( value & 0xf0 );
      clear<bitV>();
      clear<bitC>();
      if ( lo > 0x09 )
      {
        hi += 0x10;
        lo += 0x06;
      }
      if ( ~( a ^ value ) & ( a ^ hi ) & 0x80 )
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
      a = ( lo & 0x0f ) + ( hi & 0xf0 );
      setnz( a );
      return true;
    }
    else
    {
      int sum = a + value + ( get<bitC>() ? 0x01 : 0 );
      clear<bitV>();
      clear<bitC>();
      if ( ~( a ^ value ) & ( a ^ sum ) & 0x80 )
      {
        set<bitV>();
      }
      if ( sum & 0xff00 )
      {
        set<bitC>();
      }
      a = ( uint8_t )sum;
      setnz( a );
    }
    break;
  case Opcode::RZP_AND:
  case Opcode::RZX_AND:
  case Opcode::RIN_AND:
  case Opcode::RIX_AND:
  case Opcode::RIY_AND:
  case Opcode::RAB_AND:
  case Opcode::RAX_AND:
  case Opcode::RAY_AND:
  case Opcode::IMM_AND:
    setnz( a &= value );
    break;
  case Opcode::RZP_BIT:
  case Opcode::RZX_BIT:
  case Opcode::RAB_BIT:
  case Opcode::RAX_BIT:
    setz( a & value );
    set<bitN>( ( value & 0x80 ) != 0x00 );
    set<bitV>( ( value & 0x40 ) != 0x00 );
    break;
  case Opcode::IMM_BIT:
    setz( a & value );
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
    set<bitC>( a >= value );
    setnz( a - value );;
    break;
  case Opcode::RZP_CPX:
  case Opcode::RAB_CPX:
  case Opcode::IMM_CPX:
    set<bitC>( x >= value );
    setnz( x - value );
    break;
  case Opcode::RZP_CPY:
  case Opcode::RAB_CPY:
  case Opcode::IMM_CPY:
    set<bitC>( y >= value );
    setnz( y - value );
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
    setnz( a ^= value );
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
    setnz( a = value );
    break;
  case Opcode::RAY_LDX:
  case Opcode::RZP_LDX:
  case Opcode::RAB_LDX:
  case Opcode::IMM_LDX:
    setnz( x = value );
    break;
  case Opcode::RZP_LDY:
  case Opcode::RZX_LDY:
  case Opcode::RAB_LDY:
  case Opcode::RAX_LDY:
  case Opcode::IMM_LDY:
    setnz( y = value );
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
    setnz( a |= value );
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
    if ( get<bitD>() )
    {
      int c = get<bitC>() ? 0 : 1;
      int sum = a - value - c;
      int lo = ( a & 0x0f ) - ( value & 0x0f ) - c;
      int hi = ( a & 0xf0 ) - ( value & 0xf0 );
      clear<bitV>();
      clear<bitC>();
      if ( ( a ^ value ) & ( a ^ sum ) & 0x80 )
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
      a = ( lo & 0x0f ) + ( hi & 0xf0 );
      setnz( a );
      return true;
    }
    else
    {
      int c = get<bitC>() ? 0 : 1;
      int sum = a - value - c;
      clear<bitV>();
      clear<bitC>();
      if ( ( a ^ value ) & ( a ^ sum ) & 0x80 )
      {
        set<bitV>();
      }
      if ( ( sum & 0xff00 ) == 0 )
      {
        set<bitC>();
      }
      a = ( uint8_t )sum;
      setnz( a );
    }
    break;
  default:
    assert( false );
    break;
  }

  return false;
}

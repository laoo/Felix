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
  Opcode opcode{};
  for ( ;; )
  {
    union
    {
      uint16_t ea;
      struct
      {
        uint8_t eal;
        uint8_t eah;
      };
    };

    if ( cpu.interrupt && ( !cpu.I || ( cpu.interrupt & ~CPU::I_IRQ ) != 0 ) )
    {
      opcode = Opcode::BRK_BRK;
    }

    switch ( opcode )
    {
    case Opcode::RZP_ADC:
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
    case Opcode::RZP_SBC:
      break;
    case Opcode::WZP_STA:
    case Opcode::WZP_STX:
    case Opcode::WZP_STY:
    case Opcode::WZP_STZ:
      break;
    case Opcode::MZP_ASL:
    case Opcode::MZP_DEC:
    case Opcode::MZP_INC:
    case Opcode::MZP_LSR:
    case Opcode::MZP_RMB0:
    case Opcode::MZP_RMB1:
    case Opcode::MZP_RMB2:
    case Opcode::MZP_RMB3:
    case Opcode::MZP_RMB4:
    case Opcode::MZP_RMB5:
    case Opcode::MZP_RMB6:
    case Opcode::MZP_RMB7:
    case Opcode::MZP_ROL:
    case Opcode::MZP_ROR:
    case Opcode::MZP_SMB0:
    case Opcode::MZP_SMB1:
    case Opcode::MZP_SMB2:
    case Opcode::MZP_SMB3:
    case Opcode::MZP_SMB4:
    case Opcode::MZP_SMB5:
    case Opcode::MZP_SMB6:
    case Opcode::MZP_SMB7:
    case Opcode::MZP_TRB:
    case Opcode::MZP_TSB:
      break;
    case Opcode::RZX_ADC:
    case Opcode::RZX_AND:
    case Opcode::RZX_BIT:
    case Opcode::RZX_CMP:
    case Opcode::RZX_EOR:
    case Opcode::RZX_LDA:
    case Opcode::RZX_LDY:
    case Opcode::RZX_ORA:
    case Opcode::RZX_SBC:
      break;
    case Opcode::RZY_LDX:
      break;
    case Opcode::WZX_STA:
    case Opcode::WZX_STY:
    case Opcode::WZX_STZ:
      break;
    case Opcode::WZY_STX:
      break;
    case Opcode::MZX_ASL:
    case Opcode::MZX_DEC:
    case Opcode::MZX_INC:
    case Opcode::MZX_LSR:
    case Opcode::MZX_ROL:
    case Opcode::MZX_ROR:
      break;
    case Opcode::RIN_ADC:
    case Opcode::RIN_AND:
    case Opcode::RIN_CMP:
    case Opcode::RIN_EOR:
    case Opcode::RIN_LDA:
    case Opcode::RIN_ORA:
    case Opcode::RIN_SBC:
      break;
    case Opcode::WIN_STA:
      break;
    case Opcode::RIX_ADC:
    case Opcode::RIX_AND:
    case Opcode::RIX_CMP:
    case Opcode::RIX_EOR:
    case Opcode::RIX_LDA:
    case Opcode::RIX_ORA:
    case Opcode::RIX_SBC:
      break;
    case Opcode::WIX_STA:
      break;
    case Opcode::RIY_ADC:
    case Opcode::RIY_AND:
    case Opcode::RIY_CMP:
    case Opcode::RIY_EOR:
    case Opcode::RIY_LDA:
    case Opcode::RIY_ORA:
    case Opcode::RIY_SBC:
      break;
    case Opcode::WIY_STA:
      break;
    case Opcode::RAB_ADC:
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
    case Opcode::RAB_SBC:
      break;
    case Opcode::WAB_STA:
    case Opcode::WAB_STX:
    case Opcode::WAB_STY:
    case Opcode::WAB_STZ:
      break;
    case Opcode::MAB_ASL:
    case Opcode::MAB_DEC:
    case Opcode::MAB_INC:
    case Opcode::MAB_LSR:
    case Opcode::MAB_ROL:
    case Opcode::MAB_ROR:
    case Opcode::MAB_TRB:
    case Opcode::MAB_TSB:
      break;
    case Opcode::RAX_ADC:
    case Opcode::RAX_AND:
    case Opcode::RAX_CMP:
    case Opcode::RAX_EOR:
    case Opcode::RAX_LDA:
    case Opcode::RAX_LDY:
    case Opcode::RAX_ORA:
    case Opcode::RAX_SBC:
    case Opcode::RAY_ADC:
    case Opcode::RAY_AND:
    case Opcode::RAY_CMP:
    case Opcode::RAY_EOR:
    case Opcode::RAY_LDA:
    case Opcode::RAY_LDX:
    case Opcode::RAY_ORA:
    case Opcode::RAY_SBC:
      break;
    case Opcode::WAX_STA:
    case Opcode::WAX_STZ:
      break;
    case Opcode::WAY_STA:
      break;
    case Opcode::MAX_ASL:
    case Opcode::MAX_DEC:
    case Opcode::MAX_INC:
    case Opcode::MAX_LSR:
    case Opcode::MAX_ROL:
    case Opcode::MAX_ROR:
      break;
    case Opcode::JMA_JMP:
      break;
    case Opcode::JSA_JSR:
      break;
    case Opcode::JMX_JMP:
      break;
    case Opcode::JMI_JMP:
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
      break;
    case Opcode::IMM_ADC:
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
    case Opcode::IMM_SBC:
      break;
    case Opcode::BRL_BCC:
    case Opcode::BRL_BCS:
    case Opcode::BRL_BEQ:
    case Opcode::BRL_BMI:
    case Opcode::BRL_BNE:
    case Opcode::BRL_BPL:
    case Opcode::BRL_BRA:
    case Opcode::BRL_BVC:
    case Opcode::BRL_BVS:
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
      break;
    case Opcode::BRK_BRK:
      co_yield{ cpu.pc };
      if ( cpu.interrupt & CPU::I_RESET )
      {
        co_yield{ cpu.s-- };
        co_yield{ cpu.s-- };
        co_yield{ cpu.s-- };
        eal = co_yield{ 0xfffc };
        eah = co_yield{ 0xfffd };
        cpu.interrupt &= ~CPU::I_RESET;
      }
      else
      {
        co_yield{ cpu.s--, cpu.pch };
        co_yield{ cpu.s--, cpu.pcl };
        if ( cpu.interrupt == CPU::I_NONE )
        {
          co_yield{ cpu.s--, cpu.pbrk() };
        }
        else
        {
          co_yield{ cpu.s--, cpu.p() };
        }
        if ( cpu.interrupt & CPU::I_NMI )
        {
          eal = co_yield{ 0xfffa };
          eah = co_yield{ 0xfffb };
          cpu.interrupt &= ~CPU::I_NMI;
        }
        else
        {
          eal = co_yield{ 0xfffe };
          eah = co_yield{ 0xffff };
          if ( cpu.interrupt == CPU::I_IRQ )
          {
            cpu.interrupt &= ~CPU::I_IRQ;
          }
        }
      }
      cpu.pc = ea;
     break;
    case Opcode::RTI_RTI:
      break;
    case Opcode::RTS_RTS:
      break;
    case Opcode::PHR_PHA:
    case Opcode::PHR_PHP:
    case Opcode::PHR_PHX:
    case Opcode::PHR_PHY:
      break;
    case Opcode::PLR_PLA:
    case Opcode::PLR_PLP:
    case Opcode::PLR_PLX:
    case Opcode::PLR_PLY:
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
    case Opcode::UND_4_54:
    case Opcode::UND_4_d4:
    case Opcode::UND_4_f4:
    case Opcode::UND_4_dc:
    case Opcode::UND_4_fc:
    case Opcode::UND_8_5c:
      break;
    }

    for ( ;; )
    {
      uint8_t op = co_yield{ cpu.pc };
      opcode = (Opcode)op;
      if ( isHiccup( opcode ) )
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

AwaitWrite CpuLoop::promise_type::yield_value( Write w )
{
  return AwaitWrite{ mBus->request( w ) };
}

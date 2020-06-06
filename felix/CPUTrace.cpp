#include "CPUTrace.hpp"
#include "Felix.hpp"
#include "Opcodes.hpp"
#include <cassert>
#include <cstdio>
#include <Windows.h>
#include <fstream>

std::ofstream fout{ "d:/out.txt" };


CpuTrace cpuTrace( CPU & cpu, TraceRequest & req )
{
  AwaitDisasmFetch adf{ req };

  for ( ;; )
  {
    char buf[256];
    uint8_t lo;
    uint8_t hi;

    int off = sprintf( buf, "%llu: PC:%04x A:%02x X:%02x Y:%02x S:%04x P:%c%c1%c%c%c%c%c ", cpu.state.tick, (uint16_t)(cpu.state.pc-1), cpu.state.a, cpu.state.x, cpu.state.y, cpu.state.s, ( cpu.get<CPU::bitN>() ? 'N' : '-' ), ( cpu.get<CPU::bitV>() ? 'V' : '-' ), ( cpu.get<CPU::bitB>() ? 'B' : '-' ), ( cpu.get<CPU::bitD>() ? 'D' : '-' ), ( cpu.get<CPU::bitI>() ? 'I' : '-' ), ( cpu.get<CPU::bitZ>() ? 'Z' : '-' ), ( cpu.get<CPU::bitC>() ? 'C' : '-' ) );

    switch ( cpu.state.op )
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
      off += sprintf( buf + off, "and " );
      break;
    case Opcode::RZP_BIT:
    case Opcode::RZX_BIT:
    case Opcode::RAB_BIT:
    case Opcode::RAX_BIT:
    case Opcode::IMM_BIT:
      off += sprintf( buf + off, "bit " );
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
      off += sprintf( buf + off, "cmp " );
      break;
    case Opcode::RZP_CPX:
    case Opcode::RAB_CPX:
    case Opcode::IMM_CPX:
      off += sprintf( buf + off, "cpx " );
      break;
    case Opcode::RZP_CPY:
    case Opcode::RAB_CPY:
    case Opcode::IMM_CPY:
      off += sprintf( buf + off, "cpy " );
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
      off += sprintf( buf + off, "eor " );
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
      off += sprintf( buf + off, "lda " );
      break;
    case Opcode::RZP_LDX:
    case Opcode::RZY_LDX:
    case Opcode::RAB_LDX:
    case Opcode::RAY_LDX:
    case Opcode::IMM_LDX:
      off += sprintf( buf + off, "ldx " );
      break;
    case Opcode::RZP_LDY:
    case Opcode::RZX_LDY:
    case Opcode::RAB_LDY:
    case Opcode::RAX_LDY:
    case Opcode::IMM_LDY:
      off += sprintf( buf + off, "ldy " );
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
      off += sprintf( buf + off, "ora " );
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
      off += sprintf( buf + off, "adc " );
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
      off += sprintf( buf + off, "sbc " );
      break;
    case Opcode::WZP_STA:
    case Opcode::WZX_STA:
    case Opcode::WIN_STA:
    case Opcode::WIX_STA:
    case Opcode::WIY_STA:
    case Opcode::WAB_STA:
    case Opcode::WAX_STA:
    case Opcode::WAY_STA:
      off += sprintf( buf + off, "sta " );
      break;
    case Opcode::WZP_STX:
    case Opcode::WZY_STX:
    case Opcode::WAB_STX:
      off += sprintf( buf + off, "stx " );
      break;
    case Opcode::WZP_STY:
    case Opcode::WZX_STY:
    case Opcode::WAB_STY:
      off += sprintf( buf + off, "sty " );
      break;
    case Opcode::WZP_STZ:
    case Opcode::WZX_STZ:
    case Opcode::WAB_STZ:
    case Opcode::WAX_STZ:
      off += sprintf( buf + off, "stz " );
      break;
    case Opcode::MZP_ASL:
    case Opcode::MZX_ASL:
    case Opcode::MAB_ASL:
    case Opcode::MAX_ASL:
      off += sprintf( buf + off, "asl " );
      break;
    case Opcode::IMP_ASL:
      off += sprintf( buf + off, "asl\n" );
      break;
    case Opcode::MZP_DEC:
    case Opcode::MZX_DEC:
    case Opcode::MAB_DEC:
    case Opcode::MAX_DEC:
      off += sprintf( buf + off, "dec " );
      break;
    case Opcode::IMP_DEC:
      off += sprintf( buf + off, "dec\n" );
      break;
    case Opcode::MZP_INC:
    case Opcode::MZX_INC:
    case Opcode::MAB_INC:
    case Opcode::MAX_INC:
      off += sprintf( buf + off, "inc " );
      break;
    case Opcode::IMP_INC:
      off += sprintf( buf + off, "inc\n" );
      break;
    case Opcode::MZP_LSR:
    case Opcode::MZX_LSR:
    case Opcode::MAB_LSR:
    case Opcode::MAX_LSR:
      off += sprintf( buf + off, "lsr " );
      break;
    case Opcode::IMP_LSR:
      off += sprintf( buf + off, "lsr\n" );
      break;
    case Opcode::MZP_ROL:
    case Opcode::MZX_ROL:
    case Opcode::MAB_ROL:
    case Opcode::MAX_ROL:
      off += sprintf( buf + off, "rol " );
      break;
    case Opcode::IMP_ROL:
      off += sprintf( buf + off, "rol\n" );
      break;
    case Opcode::MZP_ROR:
    case Opcode::MZX_ROR:
    case Opcode::MAB_ROR:
    case Opcode::MAX_ROR:
      off += sprintf( buf + off, "ror " );
      break;
    case Opcode::IMP_ROR:
      off += sprintf( buf + off, "ror\n" );
      break;
    case Opcode::MZP_TRB:
    case Opcode::MAB_TRB:
      off += sprintf( buf + off, "trb " );
      break;
    case Opcode::MZP_TSB:
    case Opcode::MAB_TSB:
      off += sprintf( buf + off, "tsb " );
      break;
    case Opcode::MZP_RMB0:
      off += sprintf( buf + off, "rmb0 " );
      break;
    case Opcode::MZP_RMB1:
      off += sprintf( buf + off, "rmb1 " );
      break;
    case Opcode::MZP_RMB2:
      off += sprintf( buf + off, "rmb2 " );
      break;
    case Opcode::MZP_RMB3:
      off += sprintf( buf + off, "rmb3 " );
      break;
    case Opcode::MZP_RMB4:
      off += sprintf( buf + off, "rmb4 " );
      break;
    case Opcode::MZP_RMB5:
      off += sprintf( buf + off, "rmb5 " );
      break;
    case Opcode::MZP_RMB6:
      off += sprintf( buf + off, "rmb6 " );
      break;
    case Opcode::MZP_RMB7:
      off += sprintf( buf + off, "rmb7 " );
      break;
    case Opcode::MZP_SMB0:
      off += sprintf( buf + off, "smb0 " );
      break;
    case Opcode::MZP_SMB1:
      off += sprintf( buf + off, "smb1 " );
      break;
    case Opcode::MZP_SMB2:
      off += sprintf( buf + off, "smb2 " );
      break;
    case Opcode::MZP_SMB3:
      off += sprintf( buf + off, "smb3 " );
      break;
    case Opcode::MZP_SMB4:
      off += sprintf( buf + off, "smb4 " );
      break;
    case Opcode::MZP_SMB5:
      off += sprintf( buf + off, "smb5 " );
      break;
    case Opcode::MZP_SMB6:
      off += sprintf( buf + off, "smb6 " );
      break;
    case Opcode::MZP_SMB7:
      off += sprintf( buf + off, "smb7 " );
      break;
    case Opcode::JMA_JMP:
    case Opcode::JMX_JMP:
    case Opcode::JMI_JMP:
      off += sprintf( buf + off, "jmp " );
      break;
    case Opcode::JSA_JSR:
      off += sprintf( buf + off, "jsr " );
      break;
    case Opcode::IMP_CLC:
      off += sprintf( buf + off, "clc\n" );
      break;
    case Opcode::IMP_CLD:
      off += sprintf( buf + off, "cld\n" );
      break;
    case Opcode::IMP_CLI:
      off += sprintf( buf + off, "cli\n" );
      break;
    case Opcode::IMP_CLV:
      off += sprintf( buf + off, "clv\n" );
      break;
    case Opcode::IMP_DEX:
      off += sprintf( buf + off, "dex\n" );
      break;
    case Opcode::IMP_DEY:
      off += sprintf( buf + off, "dey\n" );
      break;
    case Opcode::IMP_INX:
      off += sprintf( buf + off, "inx\n" );
      break;
    case Opcode::IMP_INY:
      off += sprintf( buf + off, "iny\n" );
      break;
    case Opcode::IMP_NOP:
      off += sprintf( buf + off, "nop\n" );
      break;
    case Opcode::UND_2_02:
    case Opcode::UND_2_22:
    case Opcode::UND_2_42:
    case Opcode::UND_2_62:
    case Opcode::UND_2_82:
    case Opcode::UND_2_C2:
    case Opcode::UND_2_E2:
      off += sprintf( buf + off, "nop " );
      break;
    case Opcode::IMP_SEC:
      off += sprintf( buf + off, "sec\n" );
      break;
    case Opcode::IMP_SED:
      off += sprintf( buf + off, "sed\n" );
      break;
    case Opcode::IMP_SEI:
      off += sprintf( buf + off, "sei\n" );
      break;
    case Opcode::IMP_TAX:
      off += sprintf( buf + off, "tax\n" );
      break;
    case Opcode::IMP_TAY:
      off += sprintf( buf + off, "tay\n" );
      break;
    case Opcode::IMP_TSX:
      off += sprintf( buf + off, "tsx\n" );
      break;
    case Opcode::IMP_TXA:
      off += sprintf( buf + off, "txa\n" );
      break;
    case Opcode::IMP_TXS:
      off += sprintf( buf + off, "txs\n" );
      break;
    case Opcode::IMP_TYA:
      off += sprintf( buf + off, "tya\n" );
      break;
    case Opcode::BRL_BCC:
      off += sprintf( buf + off, "bcc " );
      break;
    case Opcode::BRL_BCS:
      off += sprintf( buf + off, "bcs " );
      break;
    case Opcode::BRL_BEQ:
      off += sprintf( buf + off, "beq " );
      break;
    case Opcode::BRL_BMI:
      off += sprintf( buf + off, "bmi " );
      break;
    case Opcode::BRL_BNE:
      off += sprintf( buf + off, "bne " );
      break;
    case Opcode::BRL_BPL:
      off += sprintf( buf + off, "bpl " );
      break;
    case Opcode::BRL_BRA:
      off += sprintf( buf + off, "bra " );
      break;
    case Opcode::BRL_BVC:
      off += sprintf( buf + off, "bvc " );
      break;
    case Opcode::BRL_BVS:
      off += sprintf( buf + off, "bvs " );
      break;
    case Opcode::BZR_BBR0:
      off += sprintf( buf + off, "bbr0 " );
      break;
    case Opcode::BZR_BBR1:
      off += sprintf( buf + off, "bbr1 " );
      break;
    case Opcode::BZR_BBR2:
      off += sprintf( buf + off, "bbr2 " );
      break;
    case Opcode::BZR_BBR3:
      off += sprintf( buf + off, "bbr3 " );
      break;
    case Opcode::BZR_BBR4:
      off += sprintf( buf + off, "bbr4 " );
      break;
    case Opcode::BZR_BBR5:
      off += sprintf( buf + off, "bbr5 " );
      break;
    case Opcode::BZR_BBR6:
      off += sprintf( buf + off, "bbr6 " );
      break;
    case Opcode::BZR_BBR7:
      off += sprintf( buf + off, "bbr7 " );
      break;
    case Opcode::BZR_BBS0:
      off += sprintf( buf + off, "bbs0 " );
      break;
    case Opcode::BZR_BBS1:
      off += sprintf( buf + off, "bbs1 " );
      break;
    case Opcode::BZR_BBS2:
      off += sprintf( buf + off, "bbs2 " );
      break;
    case Opcode::BZR_BBS3:
      off += sprintf( buf + off, "bbs3 " );
      break;
    case Opcode::BZR_BBS4:
      off += sprintf( buf + off, "bbs4 " );
      break;
    case Opcode::BZR_BBS5:
      off += sprintf( buf + off, "bbs5 " );
      break;
    case Opcode::BZR_BBS6:
      off += sprintf( buf + off, "bbs6 " );
      break;
    case Opcode::BZR_BBS7:
      off += sprintf( buf + off, "bbs7 " );
      break;
    case Opcode::BRK_BRK:
      if ( ( cpu.state.interrupt & CPU::I_RESET ) != 0 )
      {
        off += sprintf( buf + off, "RESET\n" );
      }
      else if ( ( cpu.state.interrupt & CPU::I_NMI ) != 0 )
      {
        off += sprintf( buf + off, "NMI\n" );
      }
      else if ( ( cpu.state.interrupt & CPU::I_IRQ ) != 0 )
      {
        off += sprintf( buf + off, "IRQ\n" );
      }
      else
      {
        off += sprintf( buf + off, "brk\n" );
      }
      break;
    case Opcode::RTI_RTI:
      off += sprintf( buf + off, "rti\n" );
      break;
    case Opcode::RTS_RTS:
      off += sprintf( buf + off, "rts\n" );
      break;
    case Opcode::PHR_PHA:
      off += sprintf( buf + off, "pha\n" );
      break;
    case Opcode::PHR_PHP:
      off += sprintf( buf + off, "php\n" );
      break;
    case Opcode::PHR_PHX:
      off += sprintf( buf + off, "phx\n" );
      break;
    case Opcode::PHR_PHY:
      off += sprintf( buf + off, "phy\n" );
      break;
    case Opcode::PLR_PLA:
      off += sprintf( buf + off, "pla\n" );
      break;
    case Opcode::PLR_PLP:
      off += sprintf( buf + off, "plp\n" );
      break;
    case Opcode::PLR_PLX:
      off += sprintf( buf + off, "plx\n" );
      break;
    case Opcode::PLR_PLY:
      off += sprintf( buf + off, "ply\n" );
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
      off += sprintf( buf + off, "nop\n" );
      break;
    case Opcode::UND_3_44:
    case Opcode::UND_4_54:
    case Opcode::UND_4_d4:
    case Opcode::UND_4_f4:
    case Opcode::UND_4_dc:
    case Opcode::UND_4_fc:
    case Opcode::UND_8_5c:
      off += sprintf( buf + off, "nop " );
      break;
    }


    switch ( cpu.state.op )
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
    case Opcode::RZP_ADC:
    case Opcode::RZP_SBC:
    case Opcode::WZP_STA:
    case Opcode::WZP_STX:
    case Opcode::WZP_STY:
    case Opcode::WZP_STZ:
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
    case Opcode::UND_3_44:
      co_await adf;
      lo = cpu.operand;
      sprintf( buf + off, "$%02x\n", lo );
      break;
    case Opcode::RZX_AND:
    case Opcode::RZX_BIT:
    case Opcode::RZX_CMP:
    case Opcode::RZX_EOR:
    case Opcode::RZX_LDA:
    case Opcode::RZX_LDY:
    case Opcode::RZX_ORA:
    case Opcode::RZX_ADC:
    case Opcode::RZX_SBC:
    case Opcode::WZX_STA:
    case Opcode::WZX_STY:
    case Opcode::WZX_STZ:
    case Opcode::MZX_ASL:
    case Opcode::MZX_DEC:
    case Opcode::MZX_INC:
    case Opcode::MZX_LSR:
    case Opcode::MZX_ROL:
    case Opcode::MZX_ROR:
    case Opcode::UND_4_54:
    case Opcode::UND_4_d4:
    case Opcode::UND_4_f4:
      co_await adf;
      lo = cpu.operand;
      sprintf( buf + off, "$%02x,x\n", lo );
      break;
    case Opcode::RZY_LDX:
    case Opcode::WZY_STX:
      co_await adf;
      lo = cpu.operand;
      sprintf( buf + off, "$%02x,y\n", lo );
      break;
    case Opcode::RIN_AND:
    case Opcode::RIN_CMP:
    case Opcode::RIN_EOR:
    case Opcode::RIN_LDA:
    case Opcode::RIN_ORA:
    case Opcode::RIN_ADC:
    case Opcode::RIN_SBC:
    case Opcode::WIN_STA:
      co_await adf;
      lo = cpu.operand;
      sprintf( buf + off, "($%02x)\n", lo );
      break;
    case Opcode::RIX_AND:
    case Opcode::RIX_CMP:
    case Opcode::RIX_EOR:
    case Opcode::RIX_LDA:
    case Opcode::RIX_ORA:
    case Opcode::RIX_ADC:
    case Opcode::RIX_SBC:
    case Opcode::WIX_STA:
      co_await adf;
      lo = cpu.operand;
      sprintf( buf + off, "($%02x,x)\n", lo );
      break;
    case Opcode::RIY_AND:
    case Opcode::RIY_CMP:
    case Opcode::RIY_EOR:
    case Opcode::RIY_LDA:
    case Opcode::RIY_ORA:
    case Opcode::RIY_ADC:
    case Opcode::RIY_SBC:
    case Opcode::WIY_STA:
      co_await adf;
      lo = cpu.operand;
      sprintf( buf + off, "($%02x),y\n", lo );
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
    case Opcode::WAB_STA:
    case Opcode::WAB_STX:
    case Opcode::WAB_STY:
    case Opcode::WAB_STZ:
    case Opcode::MAB_ASL:
    case Opcode::MAB_DEC:
    case Opcode::MAB_INC:
    case Opcode::MAB_LSR:
    case Opcode::MAB_ROL:
    case Opcode::MAB_ROR:
    case Opcode::MAB_TRB:
    case Opcode::MAB_TSB:
    case Opcode::JMA_JMP:
    case Opcode::JSA_JSR:
    case Opcode::UND_4_dc:
    case Opcode::UND_4_fc:
    case Opcode::UND_8_5c:
      co_await adf;
      lo = cpu.operand;
      co_await adf;
      hi = cpu.operand;
      sprintf( buf + off, "$%02x%02x\n", hi, lo );
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
    case Opcode::WAX_STA:
    case Opcode::WAX_STZ:
    case Opcode::MAX_ASL:
    case Opcode::MAX_DEC:
    case Opcode::MAX_INC:
    case Opcode::MAX_LSR:
    case Opcode::MAX_ROL:
    case Opcode::MAX_ROR:
      co_await adf;
      lo = cpu.operand;
      co_await adf;
      hi = cpu.operand;
      sprintf( buf + off, "$%02x%02x,x\n", hi, lo );
      break;
    case Opcode::RAY_AND:
    case Opcode::RAY_CMP:
    case Opcode::RAY_EOR:
    case Opcode::RAY_LDA:
    case Opcode::RAY_LDX:
    case Opcode::RAY_ORA:
    case Opcode::RAY_ADC:
    case Opcode::RAY_SBC:
    case Opcode::WAY_STA:
      co_await adf;
      lo = cpu.operand;
      co_await adf;
      hi = cpu.operand;
      sprintf( buf + off, "$%02x%02x,y\n", hi, lo );
      break;
    case Opcode::JMX_JMP:
      co_await adf;
      lo = cpu.operand;
      co_await adf;
      hi = cpu.operand;
      sprintf( buf + off, "($%02x%02x,x)\n", hi, lo );
      break;
    case Opcode::JMI_JMP:
      co_await adf;
      lo = cpu.operand;
      co_await adf;
      hi = cpu.operand;
      sprintf( buf + off, "($%02x%02x)\n", hi, lo );
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
      co_await adf;
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
      co_await adf;
      lo = cpu.operand;
      sprintf( buf + off, "#$%02x\n", lo );
      break;
    case Opcode::BRL_BCC:
    case Opcode::BRL_BCS:
    case Opcode::BRL_BEQ:
    case Opcode::BRL_BMI:
    case Opcode::BRL_BNE:
    case Opcode::BRL_BPL:
    case Opcode::BRL_BVC:
    case Opcode::BRL_BVS:
      co_await adf;
      lo = cpu.operand;
      sprintf( buf + off, "$%04x\n", (uint16_t)( cpu.state.pc - 1 + (int8_t)lo ) );
      break;
    case Opcode::BRL_BRA:
      co_await adf;
      lo = cpu.operand;
      sprintf( buf + off, "$%04x\n", ( uint16_t )( cpu.state.pc + ( int8_t )lo ) );
      break;
    case Opcode::BZR_BBR0:
    case Opcode::BZR_BBR1:
    case Opcode::BZR_BBR2:
    case Opcode::BZR_BBR3:
    case Opcode::BZR_BBR4:
    case Opcode::BZR_BBR5:
    case Opcode::BZR_BBR6:
    case Opcode::BZR_BBR7:
      co_await adf;
      lo = cpu.operand;
      co_await adf;
      hi = cpu.operand;
      sprintf( buf + off, "$%02x,$%04x\n", lo, ( uint16_t )( cpu.state.pc - 6 + ( int8_t )hi ) );
      break;
    case Opcode::BZR_BBS0:
    case Opcode::BZR_BBS1:
    case Opcode::BZR_BBS2:
    case Opcode::BZR_BBS3:
    case Opcode::BZR_BBS4:
    case Opcode::BZR_BBS5:
    case Opcode::BZR_BBS6:
    case Opcode::BZR_BBS7:
      co_await adf;
      lo = cpu.operand;
      co_await adf;
      hi = cpu.operand;
      sprintf( buf + off, "$%02x,$%04x\n", lo, (uint16_t)( cpu.state.pc + (int8_t)hi ) );
      break;
    }

    fout << buf;
    //fout.flush();
    //OutputDebugStringA( buf );

    co_await adf;
  }
}





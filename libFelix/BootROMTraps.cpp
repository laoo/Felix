#include "pch.hpp"
#include "BootROMTraps.hpp"
#include "CPUState.hpp"
#include "TraceHelper.hpp"
#include "ScriptDebugger.hpp"
#include "Core.hpp"
#include "Encryption.hpp"
#include "Opcodes.hpp"
#include "Log.hpp"

namespace
{

class ResetTrap : public IMemoryAccessTrap
{
public:
  ~ResetTrap() override = default;

  uint8_t trap( Core& state, uint16_t address, uint8_t orgValue ) override
  {
    state.debugWriteMikey( 0x8b, 2 );  //IODAT
    state.debugWriteMikey( 0x8a, 3 );  //IODIR

    //jumps to clear handler which address is held in two next bytes after reset hander address
    return (uint8_t)Opcode::JMA_JMP;
  }
};

class DecryptTrap : public IMemoryAccessTrap
{
public:
  ~DecryptTrap() override = default;

  uint8_t trap( Core& state, uint16_t address, uint8_t orgValue ) override
  {
    //decrypts to buffer pointed by address (0x0005)
    uint16_t addr = state.debugReadRAM( 5 ) + ( (uint16_t)state.debugReadRAM( 6 ) << 8 );

    size_t blockcount = 0x100 - state.debugReadSuzy( 0xb2 );

    if ( blockcount > 5 )
    {
      L_ERROR << "Bad number of encrypted blocks: " << blockcount;
      //TODO: should open debug monitor etc.
      return (uint8_t)Opcode::BRK_BRK;
    }

    std::vector<uint8_t> enc;
    for ( size_t i = 0; i < 51 * blockcount; ++i )
    {
      enc.push_back( state.debugReadSuzy( 0xb2 ) );
    }

    auto plain = decrypt( blockcount, std::span<uint8_t const>{ enc.data(), enc.size() } );

    assert( plain.size() <= 50 * blockcount );

    for ( uint8_t byte : plain )
    {
      state.debugWriteRAM( addr++, byte );
    }
    //jumps to 0x200 held in two next bytes after trap hander address
    return (uint8_t)Opcode::JMA_JMP;
  }
};

class ClearTrap : public IMemoryAccessTrap
{
public:
  ~ClearTrap() override = default;

  uint8_t trap( Core& state, uint16_t address, uint8_t orgValue ) override
  {
    for ( int i = 0; i < 0x10000; ++i )
    {
      state.debugWriteRAM( (uint16_t)i, 0 );
    }

    initMikeyRegisters( state );

    //sets address to decrypt to
    state.debugWriteRAM( 5, DECRYPTED_ROM_START_ADDRESS & 0xff );
    state.debugWriteRAM( 6, DECRYPTED_ROM_START_ADDRESS >> 8 );

    //sets return address to decrypt handler
    state.debugWriteRAM( state.debugState().s--, ( BOOTROM_DECRYPT_HANDLER_ADDRESS - 1 ) >> 8 );
    state.debugWriteRAM( state.debugState().s--, ( BOOTROM_DECRYPT_HANDLER_ADDRESS - 1 ) & 0xff );

    //clears accumulator as it will jump to shift handler to select 1st cartridge block
    state.debugState().a = 0;
    //jumps to decrypt handler which address is held in two next bytes after clear hander address
    return (uint8_t)Opcode::JMA_JMP;
  }
};

class ShiftTrap : public IMemoryAccessTrap
{
public:
  ~ShiftTrap() override = default;

  uint8_t trap( Core& state, uint16_t address, uint8_t orgValue ) override
  {
    state.debugWriteMikey( 0x87, 2 );  //SYSCTL1

    uint8_t value = state.debugState().a;
    for ( int i = 0; i < 8; ++i )
    {
      state.debugWriteMikey( 0x8b, ( value & 0x80 ) != 0 ? 2 : 0 );  //IODAT
      value <<= 1;
      state.debugWriteMikey( 0x87, 3 );  //SYSCTL1
      state.debugWriteMikey( 0x87, 2 );  //SYSCTL1
    }
    //shift handler returns to caller
    return (uint8_t)Opcode::RTS_RTS;
  }
};

}

void setBootROMTraps( std::shared_ptr<TraceHelper> traceHelper, ScriptDebugger& scriptDebugger )
{
  scriptDebugger.addTrap( ScriptDebugger::Type::ROM_EXECUTE, 0xfe00, std::make_shared<ShiftTrap>() );
  scriptDebugger.addTrap( ScriptDebugger::Type::ROM_EXECUTE, 0xfe19, std::make_shared<ClearTrap>() );
  scriptDebugger.addTrap( ScriptDebugger::Type::ROM_EXECUTE, 0xfe4a, std::make_shared<DecryptTrap>() );
  scriptDebugger.addTrap( ScriptDebugger::Type::ROM_EXECUTE, 0xff80, std::make_shared<ResetTrap>() );
}

void initMikeyRegisters( Core& state )
{
  state.debugWriteMikey( 0x00, 0x9e ); //TIM0BCKUP
  state.debugWriteMikey( 0x01, 0x18 ); //TIM0CTLA
  state.debugWriteMikey( 0xa0, 0x00 ); //GREEN0
  state.debugWriteMikey( 0xb0, 0x00 ); //BLUERED0
  state.debugWriteMikey( 0xaf, 0x0e ); //GREENF
  state.debugWriteMikey( 0xbf, 0x3e ); //BLUEREDF
  state.debugWriteMikey( 0x08, 0x68 ); //TIM2BCKUP
  state.debugWriteMikey( 0x09, 0x1f ); //TIM2CTLA
  state.debugWriteMikey( 0x93, 0x29 ); //PBCKUP
  state.debugWriteMikey( 0x94, 0x00 ); //DISPADRL
  state.debugWriteMikey( 0x95, 0x20 ); //DISPADRH
  state.debugWriteMikey( 0x92, 0x0d ); //DISPCTL
  state.debugWriteMikey( 0x90, 0x00 ); //SDONEACK
}

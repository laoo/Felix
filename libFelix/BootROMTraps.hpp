#pragma once

class ScriptDebugger;
class TraceHelper;
class Core;

static constexpr uint16_t BOOTROM_RESET_HANDLER_ADDRESS = 0xff80;
static constexpr uint16_t BOOTROM_DECRYPT_HANDLER_ADDRESS = 0xfe4a;
static constexpr uint16_t BOOTROM_CLEAR_HANDLER_ADDRESS = 0xfe19;
static constexpr uint16_t BOOTROM_SHIFT_HANDLER_ADDRESS = 0xfe00;
static constexpr uint16_t DECRYPTED_ROM_START_ADDRESS = 0x0200;

void setBootROMTraps( std::shared_ptr<TraceHelper> traceHelper, ScriptDebugger& scriptDebugger );

void initMikeyRegisters( Core& state );

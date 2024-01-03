#include "pch.hpp"
#include "LuaProxies.hpp"
#include "ScriptDebuggerEscapes.hpp"
#include "Manager.hpp"
#include "SymbolSource.hpp"
#include "Core.hpp"
#include "CPUState.hpp"

void TrapProxy::set( TrapProxy& proxy, int idx, sol::function fun )
{
  struct LuaTrap : public IMemoryAccessTrap
  {
    sol::function fun;

    LuaTrap( sol::function fun ) : fun{ fun } {}
    ~LuaTrap() override = default;

    virtual uint8_t trap( Core& core, uint16_t address, uint8_t orgValue ) override
    {
      return fun( orgValue, address );
    }

    Kind getKind() const override
    {
      return LUA;
    }
  };

  if ( idx >= 0 && idx < 65536 )
  {
    proxy.scriptDebuggerEscapes->addTrap( proxy.type, (uint16_t)idx, std::make_shared<LuaTrap>( fun ) );
  }
}

sol::object RamProxy::get( sol::stack_object key, sol::this_state L )
{
  if ( auto optIdx = key.as<sol::optional<int>>() )
  {
    int idx = *optIdx;
    if ( idx >= 0 && idx < 65536 && manager.mInstance )
    {
      auto result = manager.mInstance->debugReadRAM( (uint16_t)idx );
      return sol::object( L, sol::in_place, result );
    }
  }
  else if ( auto optSt = key.as<sol::optional<std::string>>() )
  {
    const std::string& k = *optSt;

    if ( k == "r" )
    {
      return sol::object( L, sol::in_place, TrapProxy{ manager.mScriptDebuggerEscapes, ScriptDebugger::Type::RAM_READ } );
    }
    else if ( k == "w" )
    {
      return sol::object( L, sol::in_place, TrapProxy{ manager.mScriptDebuggerEscapes, ScriptDebugger::Type::RAM_WRITE } );
    }
    else if ( k == "x" )
    {
      return sol::object( L, sol::in_place, TrapProxy{ manager.mScriptDebuggerEscapes, ScriptDebugger::Type::RAM_EXECUTE } );
    }
  }

  return sol::object( L, sol::in_place, sol::lua_nil );
}

void RamProxy::set( sol::stack_object key, sol::stack_object value, sol::this_state )
{
  if ( auto optIdx = key.as<sol::optional<int>>() )
  {
    int idx = *optIdx;
    if ( idx >= 0 && idx < 65536 )
    {
      manager.mInstance->debugWriteRAM( (uint16_t)idx, value.as<uint8_t>() );
    }
  }
}

sol::object MikeyProxy::get( sol::stack_object key, sol::this_state L )
{
  if ( auto optIdx = key.as<sol::optional<int>>() )
  {
    uint16_t idx = (uint16_t)( *optIdx & 0xff );
    auto result = manager.mInstance->debugReadMikey( idx );
    return sol::object( L, sol::in_place, result );
  }
  else if ( auto optSt = key.as<sol::optional<std::string>>() )
  {
    const std::string& k = *optSt;

    if ( k == "r" )
    {
      return sol::object( L, sol::in_place, TrapProxy{ manager.mScriptDebuggerEscapes, ScriptDebugger::Type::MIKEY_READ } );
    }
    else if ( k == "w" )
    {
      return sol::object( L, sol::in_place, TrapProxy{ manager.mScriptDebuggerEscapes, ScriptDebugger::Type::MIKEY_WRITE } );
    }
  }

  return sol::object( L, sol::in_place, sol::lua_nil );
}

void MikeyProxy::set( sol::stack_object key, sol::stack_object value, sol::this_state )
{
  if ( auto optIdx = key.as<sol::optional<int>>() )
  {
    uint16_t idx = (uint16_t)( *optIdx & 0xff );
    manager.mInstance->debugWriteMikey( idx, value.as<uint8_t>() );
  }
}

sol::object SuzyProxy::get( sol::stack_object key, sol::this_state L )
{
  if ( auto optIdx = key.as<sol::optional<int>>() )
  {
    uint16_t idx = (uint16_t)( *optIdx & 0xff );
    auto result = manager.mInstance->debugReadSuzy( idx );
    return sol::object( L, sol::in_place, result );
  }
  else if ( auto optSt = key.as<sol::optional<std::string>>() )
  {
    const std::string& k = *optSt;

    if ( k == "r" )
    {
      return sol::object( L, sol::in_place, TrapProxy{ manager.mScriptDebuggerEscapes, ScriptDebugger::Type::SUZY_READ } );
    }
    else if ( k == "w" )
    {
      return sol::object( L, sol::in_place, TrapProxy{ manager.mScriptDebuggerEscapes, ScriptDebugger::Type::SUZY_WRITE } );
    }
  }

  return sol::object( L, sol::in_place, sol::lua_nil );
}

void SuzyProxy::set( sol::stack_object key, sol::stack_object value, sol::this_state )
{
  if ( auto optIdx = key.as<sol::optional<int>>() )
  {
    uint16_t idx = (uint16_t)( *optIdx & 0xff );
    manager.mInstance->debugWriteSuzy( idx, value.as<uint8_t>() );
  }
}

sol::object RomProxy::get( sol::stack_object key, sol::this_state L )
{
  if ( auto optIdx = key.as<sol::optional<int>>() )
  {
    uint16_t idx = (uint16_t)( *optIdx & 0x1ff );
    auto result = manager.mInstance->debugReadROM( idx );
  }
  else if ( auto optSt = key.as<sol::optional<std::string>>() )
  {
    const std::string& k = *optSt;

    if ( k == "r" )
    {
      return sol::object( L, sol::in_place, TrapProxy{ manager.mScriptDebuggerEscapes, ScriptDebugger::Type::ROM_READ } );
    }
    else if ( k == "w" )
    {
      return sol::object( L, sol::in_place, TrapProxy{ manager.mScriptDebuggerEscapes, ScriptDebugger::Type::ROM_WRITE } );
    }
    else if ( k == "x" )
    {
      return sol::object( L, sol::in_place, TrapProxy{ manager.mScriptDebuggerEscapes, ScriptDebugger::Type::ROM_EXECUTE } );
    }
  }

  return sol::object( L, sol::in_place, sol::lua_nil );
}

void RomProxy::set( sol::stack_object key, sol::stack_object value, sol::this_state )
{
  //ignore
}

sol::object CPUProxy::get( sol::stack_object key, sol::this_state L )
{
  if ( auto optSt = key.as<sol::optional<std::string>>() )
  {
    const std::string& k = *optSt;

    if ( k == "a" )
    {
      return sol::object( L, sol::in_place, manager.mInstance->debugState().a );
    }
    if ( k == "x" )
    {
      return sol::object( L, sol::in_place, manager.mInstance->debugState().x );
    }
    if ( k == "y" )
    {
      return sol::object( L, sol::in_place, manager.mInstance->debugState().y );
    }
    if ( k == "pc" )
    {
      return sol::object( L, sol::in_place, manager.mInstance->debugState().pc );
    }
  }

  return sol::object( L, sol::in_place, sol::lua_nil );
}

void CPUProxy::set( sol::stack_object key, sol::stack_object value, sol::this_state )
{
  if ( auto optSt = key.as<sol::optional<std::string>>() )
  {
    const std::string& k = *optSt;

    if ( k == "a" )
    {
      manager.mInstance->debugState().a = value.as<uint8_t>();
    }
    if ( k == "x" )
    {
      manager.mInstance->debugState().x = value.as<uint8_t>();
    }
    if ( k == "y" )
    {
      manager.mInstance->debugState().y = value.as<uint8_t>();
    }
  }
}

sol::object SymbolProxy::get( sol::stack_object key, sol::this_state L )
{
  if ( auto optSt = key.as<sol::optional<std::string>>() )
  {
    const std::string& k = *optSt;

    if ( manager.mSymbols )
    {
      if ( auto opt = manager.mSymbols->symbol( k ) )
      {
        return sol::object( L, sol::in_place, *opt );
      }
    }
  }

  return sol::object( L, sol::in_place, sol::lua_nil );
}

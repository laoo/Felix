#pragma once

#include "IMemoryAccessTrap.hpp"
#include "ScriptDebugger.hpp"

class ScriptDebuggerEscapes
{
  struct Escape
  {
    std::shared_ptr<IMemoryAccessTrap> trap;
    ScriptDebugger::Type type;
    uint16_t address;
  };

public:
  ScriptDebuggerEscapes() = default;
  ~ScriptDebuggerEscapes() = default;

  void readRAM( uint16_t address, std::shared_ptr<IMemoryAccessTrap> trap )
  {
    mEscapes.push_back( { std::move( trap ), ScriptDebugger::Type::RAM_READ, address } );
  }

  void writeRAM( uint16_t address, std::shared_ptr<IMemoryAccessTrap> trap )
  {
    mEscapes.push_back( { std::move( trap ), ScriptDebugger::Type::RAM_WRITE, address } );
  }

  void executeRAM( uint16_t address, std::shared_ptr<IMemoryAccessTrap> trap )
  {
    mEscapes.push_back( { std::move( trap ), ScriptDebugger::Type::RAM_EXECUTE, address } );
  }

  void readROM( uint16_t address, std::shared_ptr<IMemoryAccessTrap> trap )
  {
    mEscapes.push_back( { std::move( trap ), ScriptDebugger::Type::ROM_READ, address } );
  }

  void writeROM( uint16_t address, std::shared_ptr<IMemoryAccessTrap> trap )
  {
    mEscapes.push_back( { std::move( trap ), ScriptDebugger::Type::ROM_WRITE, address } );
  }

  void executeROM( uint16_t address, std::shared_ptr<IMemoryAccessTrap> trap )
  {
    mEscapes.push_back( { std::move( trap ), ScriptDebugger::Type::ROM_EXECUTE, address } );
  }

  void readMikey( uint16_t address, std::shared_ptr<IMemoryAccessTrap> trap )
  {
    mEscapes.push_back( { std::move( trap ), ScriptDebugger::Type::MIKEY_READ, address } );
  }

  void writeMikey( uint16_t address, std::shared_ptr<IMemoryAccessTrap> trap )
  {
    mEscapes.push_back( { std::move( trap ), ScriptDebugger::Type::MIKEY_WRITE, address } );
  }

  void readSuzy( uint16_t address, std::shared_ptr<IMemoryAccessTrap> trap )
  {
    mEscapes.push_back( { std::move( trap ), ScriptDebugger::Type::SUZY_READ, address } );
  }

  void writeSuzy( uint16_t address, std::shared_ptr<IMemoryAccessTrap> trap )
  {
    mEscapes.push_back( { std::move( trap ), ScriptDebugger::Type::SUZY_WRITE, address } );
  }

  void readMapCtl( uint16_t address, std::shared_ptr<IMemoryAccessTrap> trap )
  {
    mEscapes.push_back( { std::move( trap ), ScriptDebugger::Type::MAPCTL_READ, address } );
  }

  void writeMapCtl( uint16_t address, std::shared_ptr<IMemoryAccessTrap> trap )
  {
    mEscapes.push_back( { std::move( trap ), ScriptDebugger::Type::MAPCTL_WRITE, address } );
  }

  void populateScriptDebugger( ScriptDebugger& scriptDebugger ) const
  {
    for ( auto& esc : mEscapes )
    {
      scriptDebugger.addTrap( esc.type, esc.address, esc.trap );
    }
  }

private:

  std::vector<Escape> mEscapes;
};


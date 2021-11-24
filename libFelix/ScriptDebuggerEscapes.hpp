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

  void addTrap( ScriptDebugger::Type type, uint16_t address, std::shared_ptr<IMemoryAccessTrap> trap )
  {
    mEscapes.push_back( { std::move( trap ), type, address } );
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


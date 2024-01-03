#pragma once

#include "Editors.hpp"
#include "ScriptDebugger.hpp"
#include "Core.hpp"
#include "CPU.hpp"

class Manager;

struct UIBreakpointTrap : public IMemoryAccessTrap
{
  UIBreakpointTrap() {}
  ~UIBreakpointTrap() override = default;

  virtual uint8_t trap( Core& core, uint16_t address, uint8_t orgValue ) override
  {
    core.debugCPU().breakFromTrap();
    return orgValue;
  }

  Kind getKind() const override
  {
    return UI;
  }
};

typedef struct BreakpointItem
{
  uint32_t id = 0;
  ScriptDebugger::Type type;
  uint16_t address = 0;

  bool operator==( const BreakpointItem& b )
  {
    return id == b.id;
  }

} BreakpointItem;

class BreakpointEditor
{
public:
  BreakpointEditor();
  ~BreakpointEditor();

  void setManager( Manager* manager );
  void drawContents();
  bool enabled();
  bool hasBreapoint( uint16_t address );
  void toggleBreapoint( uint16_t address );

private:
  Manager* mManager;
  std::vector<BreakpointItem> mItems;

  char mNewItemAddrBuf[6];
  ScriptDebugger::Type mNewItemBreakpointType = ScriptDebugger::Type::RAM_EXECUTE;

  bool isReadOnly();

  void initializeExistingTraps();
  void deleteBreakpoint( uint16_t address );
  void deleteBreakpoint( const BreakpointItem* item );
  void addBreakpoint( ScriptDebugger::Type type, const char* addr );
  void addBreakpoint( ScriptDebugger::Type type, uint16_t addr );
  const char* breakpointTypeGetDesc( ScriptDebugger::Type type ) const;
};


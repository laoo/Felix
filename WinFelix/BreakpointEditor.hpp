#pragma once

#include "Editors.hpp"

class Manager;

enum BreakpointType
{
  BreakpointType_Exec = 0,
  BreakpointType_Read = 1,
  BreakpointType_Write = 2,
  BreakpointType_COUNT
};

typedef struct BreakpointItem
{
  uint32_t id = 0;
  BreakpointType type = BreakpointType_Exec;
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

private:
  Manager* mManager;
  std::vector<BreakpointItem> mItems;

  char mNewItemAddrBuf[6];
  BreakpointType mNewItemBreakpointType;

  bool isReadOnly();

  void deleteBreakpoint( const BreakpointItem* item );
  void addBreakpoint( BreakpointType type, const char* addr );
  void addBreakpoint( BreakpointType type, uint16_t addr );
  const char* breakpointTypeGetDesc( BreakpointType type ) const;
};


#pragma once

#include "Editors.hpp"
#include <imgui.h>
#include <imgui_memory_editor.h>

class Manager;

class MemEditor
{
public:
  MemEditor();
  ~MemEditor();

  void setManager( Manager* manager );
  void drawContents();
  bool enabled();

  void writeChanges( uint16_t offset, ImU8 data );

private:
  Manager* mManager;
  MemoryEditor mMemoryEditor;

  bool isReadOnly();  
};

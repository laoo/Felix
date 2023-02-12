#pragma once

#include <cstdint>
#include <imgui.h>
#include <imgui_memory_editor.h>

class Manager;

class MemEditor
{
public:
  MemEditor();

  void setManager( Manager* manager );
  void drawContents();
  bool enabled();

private:
  Manager* mManager;
  MemoryEditor mMemoryEditor;

  bool isReadOnly();
  void writeChanges( ImU8* data, size_t off, ImU8 d );
};
#pragma once

#include "Editors.hpp"
#include <imgui.h>
#include <imgui_memory_editor.h>

class Manager;

class MemEditor : public IEditor
{
public:
  MemEditor();
  ~MemEditor() override;

  void drawContents() override;
  bool enabled() override;
  void coreHasBeenReset() override;

  void writeChanges( uint16_t offset, ImU8 data );

private:
  MemoryEditor mMemoryEditor;

  bool isReadOnly();  
};

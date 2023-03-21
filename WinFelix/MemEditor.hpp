#pragma once

#include "Editors.hpp"
#include <imgui.h>
#include "imgui_memory_editor.h"

class Manager;

enum MemEditorBank
{
  MemEditorBank_MIN = 0,
  MemEditorBank_RAM = MemEditorBank_MIN,
  MemEditorBank_ROM = 1,
  MemEditorBank_Suzy = 2,
  MemEditorBank_Mikey = 3,
  MemEditorBank_MAX = 4
};

class MemEditor : public IEditor
{
public:
  MemEditor();
  ~MemEditor() override;

  void drawContents() override;
  bool enabled() override;
  void coreHasBeenReset() override;

  void writeChanges( uint16_t offset, ImU8 data );
  ImU8 readMem( uint16_t offset );

  void drawBankButton( MemEditorBank access );
  const char* bankLabel( MemEditorBank access );
  uint16_t bankUpperBound( MemEditorBank access );

private:
  MemoryEditor mMemoryEditor;
  uint32_t mBankUppberBound = 0xFFFF + 1;
  MemEditorBank mMemBank = MemEditorBank_RAM;

  bool isReadOnly();  
};

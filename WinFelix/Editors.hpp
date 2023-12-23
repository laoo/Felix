#pragma once

#include "imgui.h"
#include <cstdint>

#define REG_TXT_LEN 3
#define PS_TXT_LEN  5
#define ITEM_WIDTH  35
#define LABEL_WIDTH 16

class Manager;

class IEditor
{
public:
  virtual ~IEditor() = default;

  virtual void coreHasBeenReset() = 0;
  virtual void drawContents() = 0;
  virtual bool enabled() = 0;
  virtual void setManager( Manager* manager ) { mManager = manager; };

  enum EditorType
  {
    Breakpoint = 0,
    CPU = 1,
    Disasm = 2,
    Memory = 3,
    Watch = 4,
    EditorType_MAX
  };

  typedef struct EditorDefinition
  {
    EditorType type;
    const char* label;
    const char* shortcut;
    ImGuiKey imGuiKey;
    bool allowMultipleInstances;
  } EditorDefinition;

protected:
  Manager* mManager{};
};

static std::vector<IEditor::EditorDefinition> EditorDefinitions
{
  IEditor::EditorDefinition { IEditor::EditorType::Breakpoint, "Breakpoint Window", "Ctrl+B", ImGuiKey_B, false },
  IEditor::EditorDefinition { IEditor::EditorType::CPU, "CPU Window", "Ctrl+C", ImGuiKey_C, false },
  IEditor::EditorDefinition { IEditor::EditorType::Disasm, "Disassembly Window", "Ctrl+D", ImGuiKey_D, true },
  IEditor::EditorDefinition { IEditor::EditorType::Memory, "Memory Window", "Ctrl+N", ImGuiKey_N, true },
  IEditor::EditorDefinition { IEditor::EditorType::Watch, "Watch Window", "Ctrl+W", ImGuiKey_W, false }
};



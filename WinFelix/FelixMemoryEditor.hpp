#pragma once

#include <cstdint>
#include <imgui.h>
#include <imgui_memory_editor.h>

class Manager;

class FelixMemoryEditor
{
public:
    FelixMemoryEditor();

    void setManager(Manager* manager);
    void drawContents();
    bool enabled();

private:
    Manager* mManager;
    MemoryEditor mMemoryEditor;
    
    bool isReadOnly();
};
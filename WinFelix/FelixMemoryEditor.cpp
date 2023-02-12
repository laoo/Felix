#include "pch.hpp"
#include "FelixMemoryEditor.hpp"
#include "Manager.hpp"
#include "Core.hpp"
#include "Debugger.hpp"

FelixMemoryEditor::FelixMemoryEditor()
{
}

void FelixMemoryEditor::setManager(Manager* manager)
{
    mManager = manager;
}

bool FelixMemoryEditor::enabled()
{
    return mManager && mManager->mInstance && mManager->mDebugger.visualizeMemory;
}

bool FelixMemoryEditor::isReadOnly()
{
    if (!enabled())
    {
        return true;
    }

    return !mManager->mDebugger.isPaused();
}


void FelixMemoryEditor::drawContents()
{
    if (!enabled())
    {
        return;
    }

    auto ram = mManager->mInstance->debugRAM();
      
    mMemoryEditor.DrawContents((void*)ram, 0xFFFF);
}

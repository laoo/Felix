#include "pch.hpp"
#include "MemoryEditor.hpp"
#include "Manager.hpp"
#include "Core.hpp"
#include "Debugger.hpp"



MemEditor::MemEditor()
{
  mMemoryEditor.WriteFn = [] ( ImU8* data, size_t off, ImU8 d )
  {
    
  };
}

void MemEditor::setManager( Manager* manager )
{
  mManager = manager;
}

bool MemEditor::enabled()
{
  return mManager && mManager->mInstance && mManager->mDebugger.visualizeMemory;
}

bool MemEditor::isReadOnly()
{
  if ( !enabled() )
  {
    return true;
  }

  return !mManager->mDebugger.isPaused();
}

void MemEditor::drawContents()
{
  if ( !enabled() )
  {
    return;
  }

  auto ram = mManager->mInstance->debugRAM();

  mMemoryEditor.DrawContents( (void*)ram, 0xFFFF );
}

void MemEditor::writeChanges( ImU8* data, size_t off, ImU8 d )
{
  do
  {
    mManager->mInstance->debugWriteRAM( (uint16_t)off, data[d] );
    --d;
  } while ( d != 0 );
}
#include "pch.hpp"
#include "MemEditor.hpp"
#include "Manager.hpp"
#include "Core.hpp"
#include "Debugger.hpp"

//TODO: get rid of the below global used for the write callback.
MemEditor* gActiveMemEditor;

MemEditor::MemEditor()
{
  gActiveMemEditor = this;
  mMemoryEditor.WriteFn = [] ( ImU8* data, size_t off, ImU8 d )
  {
    gActiveMemEditor->writeChanges( (uint16_t)off, d );
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

  mMemoryEditor.ReadOnly = isReadOnly();

  mMemoryEditor.DrawContents( (void*)ram, 0xFFFF );
}

void MemEditor::writeChanges( uint16_t offset, ImU8 data )
{
    mManager->mInstance->debugWriteRAM( offset, data );
}
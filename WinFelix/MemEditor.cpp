#include "pch.hpp"
#include "MemEditor.hpp"
#include "Manager.hpp"
#include "Core.hpp"
#include "Debugger.hpp"
#include "ConfigProvider.hpp"
#include "SysConfig.hpp"

//TODO: get rid of the below global used for the write callback.
MemEditor* gActiveMemEditor;

MemEditor::MemEditor()
{
  gActiveMemEditor = this;
  mMemoryEditor.WriteFn = [] ( ImU8* data, size_t off, ImU8 d )
  {
    gActiveMemEditor->writeChanges( (uint16_t)off, d );
  };

  auto sysConfig = gConfigProvider.sysConfig();

  mMemoryEditor.OptAddrDigitsCount = sysConfig->memoryOptions.OptAddrDigitsCount;
  mMemoryEditor.OptFooterExtraHeight = sysConfig->memoryOptions.OptFooterExtraHeight;
  mMemoryEditor.OptGreyOutZeroes = sysConfig->memoryOptions.OptGreyOutZeroes;
  mMemoryEditor.OptMidColsCount = sysConfig->memoryOptions.OptMidColsCount;
  mMemoryEditor.OptShowAscii = sysConfig->memoryOptions.OptShowAscii;
  mMemoryEditor.OptShowDataPreview = sysConfig->memoryOptions.OptShowDataPreview;
  mMemoryEditor.OptShowHexII = sysConfig->memoryOptions.OptShowHexII;
  mMemoryEditor.OptShowOptions = sysConfig->memoryOptions.OptShowOptions;
  mMemoryEditor.OptUpperCaseHex = sysConfig->memoryOptions.OptUpperCaseHex;
}

MemEditor::~MemEditor()
{
  auto sysConfig = gConfigProvider.sysConfig();

  sysConfig->memoryOptions.OptAddrDigitsCount = mMemoryEditor.OptAddrDigitsCount;
  sysConfig->memoryOptions.OptFooterExtraHeight = mMemoryEditor.OptFooterExtraHeight;
  sysConfig->memoryOptions.OptGreyOutZeroes = mMemoryEditor.OptGreyOutZeroes;
  sysConfig->memoryOptions.OptMidColsCount = mMemoryEditor.OptMidColsCount;
  sysConfig->memoryOptions.OptShowAscii = mMemoryEditor.OptShowAscii;
  sysConfig->memoryOptions.OptShowDataPreview = mMemoryEditor.OptShowDataPreview;
  sysConfig->memoryOptions.OptShowHexII = mMemoryEditor.OptShowHexII;
  sysConfig->memoryOptions.OptShowOptions = mMemoryEditor.OptShowOptions;
  sysConfig->memoryOptions.OptUpperCaseHex = mMemoryEditor.OptUpperCaseHex;
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
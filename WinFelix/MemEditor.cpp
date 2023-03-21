#include "pch.hpp"
#include "MemEditor.hpp"
#include "Manager.hpp"
#include "Debugger.hpp"
#include "Core.hpp"
#include "ConfigProvider.hpp"
#include "SysConfig.hpp"
#include "Log.hpp"

MemEditor::MemEditor()
{
  mMemoryEditor.HandlersContext = this;

  mMemoryEditor.WriteFn = []( ImU8* data, size_t off, ImU8 d, void* context )
  {
    ((MemEditor*)context)->writeChanges( (uint16_t)off, d );
  };

  mMemoryEditor.ReadFn = [] ( const ImU8* data, size_t off, void* context )
  {
    return ((MemEditor*)context)->readMem( (uint16_t)off );
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

bool MemEditor::enabled()
{
  return mManager && mManager->mInstance;
}

bool MemEditor::isReadOnly()
{
  if ( !enabled() )
  {
    return true;
  }

  return !mManager->mDebugger.isPaused();
}

uint16_t MemEditor::bankUpperBound( MemEditorBank access )
{
  uint16_t bounds[] = { 0xFFFF, 0x1FF, 0xFF, 0xFF };
  IM_ASSERT( access >= MemEditorBank_MIN && access < MemEditorBank_MAX );
  return bounds[access];
}

const char* MemEditor::bankLabel( MemEditorBank access )
{
  const char* descs[] = { "RAM", "ROM", "Suzy", "Mikey"  };
  IM_ASSERT( access >= MemEditorBank_MIN && access < MemEditorBank_MAX );
  return descs[access];
}

void MemEditor::drawBankButton( MemEditorBank bank )
{
  bool pushed = false;

  if (mMemBank == bank )
  {
    ImGui::PushStyleColor( ImGuiCol_Button, ImGui::GetColorU32( ImGuiCol_ButtonHovered ) );
    pushed = true;
  }

  if ( ImGui::Button( bankLabel( bank ), ImVec2( 60, 19 ) ) )
  {
    mMemBank = bank;
    mBankUppberBound = bankUpperBound( bank ) + 1;
  }

  if ( pushed )
  {
    ImGui::PopStyleColor();
  }
}

void MemEditor::drawContents()
{
  if ( !enabled() )
  {
    return;
  }

  for ( int a = MemEditorBank_MIN; a < MemEditorBank_MAX; ++a )
  {
    ImGui::SameLine();
    drawBankButton( (MemEditorBank)a );
  }

  mMemoryEditor.ReadOnly = isReadOnly();
  mMemoryEditor.DrawContents( nullptr, mBankUppberBound );
}

void MemEditor::writeChanges( uint16_t offset, ImU8 data )
{
  switch (mMemBank)
  {
  case MemEditorBank_RAM:
    mManager->mInstance->debugWriteRAM( offset, data );
    break;
  case MemEditorBank_Mikey:
    mManager->mInstance->debugWriteMikey( offset, data );
    break;
  case MemEditorBank_Suzy:
    mManager->mInstance->debugWriteSuzy( offset, data );
    break;
  default:
    L_ERROR << "MemEditor: Can't write to bank: " << mMemBank;
    break;
  }
}

ImU8 MemEditor::readMem( uint16_t offset )
{
  switch (mMemBank)
  {
  case MemEditorBank_RAM:
    return mManager->mInstance->debugReadRAM( offset );
  case MemEditorBank_ROM:
    return mManager->mInstance->debugReadROM( offset );
  case MemEditorBank_Mikey:
    return mManager->mInstance->debugReadMikey( offset );
  case MemEditorBank_Suzy:
    return mManager->mInstance->debugReadSuzy( offset );
  default:
    return 0;
  }
}

void MemEditor::coreHasBeenReset()
{
};
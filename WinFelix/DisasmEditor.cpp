#include "pch.hpp"
#include "DisasmEditor.h"
#include "Manager.hpp"
#include "Core.hpp"
#include "CPU.hpp"
#include "CPUState.hpp"
#include "TraceHelper.hpp"
#include "ConfigProvider.hpp"
#include "SysConfig.hpp"
#include "BreakpointEditor.hpp"

DisasmEditor::DisasmEditor() : mPC{ 0 }, mFollowPC { 0 }
{
  auto sysConfig = gConfigProvider.sysConfig();

  mFollowPC = sysConfig->disasmOptions.FollowPC;
  mShowLabelsInAddrCol = sysConfig->disasmOptions.ShowLabelsInAddrCol;
}

DisasmEditor::~DisasmEditor()
{
  auto sysConfig = gConfigProvider.sysConfig();

  sysConfig->disasmOptions.FollowPC = mFollowPC;
  sysConfig->disasmOptions.ShowLabelsInAddrCol = mShowLabelsInAddrCol;
}

void DisasmEditor::setManager( Manager* manager )
{
  mManager = manager;
}

bool DisasmEditor::enabled()
{
  return mManager && mManager->mInstance && mManager->mDebugger.visualizeDisasm && mManager->mInstance->tick() > 0;
}

bool DisasmEditor::isReadOnly()
{
  if ( !enabled() )
  {
    return true;
  }

  return !mManager->mDebugger.isPaused();
}

void DisasmEditor::drawContents()
{
  if ( !enabled() )
  {
    return;
  }

  ImGui::AlignTextToFramePadding();
  
  drawTable();
  drawOptions();
}

void DisasmEditor::drawTable()
{
  char buf[100];
  auto& cpu = mManager->mInstance->debugCPU();
  auto ram = mManager->mInstance->debugRAM();
  auto opColor = IM_COL32( 126, 88, 137, 255 );
  auto tableSize = ImGui::GetWindowSize();
  tableSize.y -= ImGuiStyleVar_CellPadding * 3;
  int itemCount = (tableSize.y / ImGuiStyleVar_CellPadding) + 1;
  uint8_t oprLength = 0;
  int prevPC;

  mPC = cpu.state().pc;
  if ( mFollowPC )
  {
    mTablePC = mPC;
  }

  int workingPc = mTablePC;

  ImGui::BeginTable( "##DisasmTable", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp, tableSize );

  ImGui::TableSetupColumn( "##disasmtableaddr", ImGuiTableColumnFlags_NoHeaderLabel, 0.2F );
  ImGui::TableSetupColumn( "##disasmtablehex", ImGuiTableColumnFlags_NoHeaderLabel, 0.2F );
  ImGui::TableSetupColumn( "##disasmtableop", ImGuiTableColumnFlags_NoHeaderLabel, 0.6F );

  if ( workingPc )
  {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text( "None" );
    if ( ImGui::IsItemVisible() )
    {
      scrollUp();
    }
    ImGui::TableNextColumn();
  }

  do
  {
    ImGui::TableNextRow();

    if ( mManager->mDebugWindows.breakpointEditor.hasBreapoint( workingPc ) )
    {
      ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32( ImVec4( 255, 0, 0, 255 ) ) );
    }
    else if ( workingPc == mPC )
    {
      ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32( ImGuiCol_Button ) );
    }

    ImGui::TableNextColumn();
    if ( mShowLabelsInAddrCol )
    {
      sprintf( buf, "%s", mManager->mInstance->getTraceHelper()->addressLabel( workingPc ) );
      ImGui::Text( buf );
      if ( ImGui::IsItemHovered() )
      {
        sprintf( buf+50, "%s ($%04X)", buf, (uint16_t)workingPc );
        ImGui::SetTooltip( buf+50 );
      }
    }
    else
    {
      sprintf( buf, "$%04X", (uint16_t)workingPc );
      ImGui::Text( buf );
    }
    if ( ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked( 0 ) )
    {
      mManager->mDebugWindows.breakpointEditor.toggleBreapoint( workingPc );
    }

    memset( buf, 0, 50 );
    prevPC = workingPc;
    cpu.disasmOp( buf, (Opcode)ram[workingPc] );
    oprLength = cpu.disasmOpr( ram, (char*)buf+20, workingPc );

    ImGui::TableNextColumn();
    const uint8_t* ram = mManager->mInstance->debugRAM();
    sprintf( buf+10, "%02X", ram[prevPC++]);
    for (uint8_t i = 0; i < oprLength; ++i )
    {
      sprintf( buf + 12 + i * 3, " %02X", ram[prevPC++] );
    }

    ImGui::Text( buf + 10 );

    ImGui::TableNextColumn();
    ImGui::PushStyleColor( ImGuiCol_Text, opColor );
    ImGui::Text( buf );
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::Text( buf + 20 );

    if ( workingPc < 0xffff && !itemCount && ImGui::IsItemVisible() )
    {
      scrollDown();
    }

  } while ( --itemCount >= 0 );

  ImGui::SetScrollY( ImGuiStyleVar_CellPadding );

  ImGui::EndTable();
  
}

void DisasmEditor::drawOptions()
{
  char addrbuf[5]{};

  ImGui::SetCursorPosY( ImGui::GetWindowHeight() - ImGuiStyleVar_CellPadding - 10 );
  ImGui::Separator();
  if ( ImGui::Button( "Options" ) )
  {
    ImGui::OpenPopup( "disasmcontext" );
  }
  if ( ImGui::BeginPopup( "disasmcontext" ) )
  {
    ImGui::Checkbox( "Follow PC", &mFollowPC );
    ImGui::Checkbox( "Show labels in address column", &mShowLabelsInAddrCol );
    ImGui::EndPopup();
  }
  ImGui::SameLine();
  ImGui::SetNextItemWidth( 40 );
  if ( ImGui::InputText( "##disasmtableaddr", addrbuf, 5, ( mFollowPC ? ImGuiInputTextFlags_ReadOnly : 0 ) | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_EnterReturnsTrue ) )
  {
    sscanf( addrbuf, "%04X", &mTablePC );
  }
}

void DisasmEditor::scrollUp()
{
  char buf[50]{};
  int prevPC;

  auto& cpu = mManager->mInstance->debugCPU();
  auto ram = mManager->mInstance->debugRAM();

  for ( char i = 1; i < 4; ++i )
  {
    prevPC = mTablePC - i;
    if ( !cpu.disasmOp( buf, (Opcode)ram[prevPC] ) )
    {
      continue;
    }
    cpu.disasmOpr( ram, (char*)buf + 5, prevPC );

    if ( mTablePC == prevPC )
    {
      mTablePC -= i;
      return;
    }
  }

  for ( char i = 1; i < 4; ++i )
  {
    prevPC = mTablePC - i;
    if ( !cpu.disasmOp( buf, (Opcode)ram[prevPC] ) )
    {
      continue;
    }
    if (i == cpu.disasmOpr( ram, (char*)buf + 5, prevPC ))
    {
      mTablePC -= i;
      return;
    }
  }

  --mTablePC;
}

void DisasmEditor::scrollDown()
{
  char buf[50];
  auto& cpu = mManager->mInstance->debugCPU();
  auto ram = mManager->mInstance->debugRAM();
  
  cpu.disasmOp( buf, (Opcode)ram[mTablePC] );
  cpu.disasmOpr( ram, (char*)buf, mTablePC );
}
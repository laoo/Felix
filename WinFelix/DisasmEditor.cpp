#include "pch.hpp"
#include "DisasmEditor.h"
#include "Manager.hpp"
#include "Core.hpp"
#include "CPU.hpp"
#include "CPUState.hpp"
#include "TraceHelper.hpp"

DisasmEditor::DisasmEditor()
{
}

DisasmEditor::~DisasmEditor()
{
}

void DisasmEditor::setManager( Manager* manager )
{
  mManager = manager;
}

bool DisasmEditor::enabled()
{
  return mManager && mManager->mInstance && mManager->mDebugger.visualizeDisasm;
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

  char buf[100];
  auto& cpu = mManager->mInstance->debugCPU();
  auto ram = mManager->mInstance->debugRAM();
  auto opColor = IM_COL32( 126, 88, 137, 255 );
  int itemCount = ImGui::GetWindowHeight() / ImGuiStyleVar_CellPadding;

  mPC = cpu.state().pc;
  mTablePC = mPC;

  ImGui::AlignTextToFramePadding();
  
  ImGui::BeginTable("##DisasmTable", 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp );
  ImGui::TableSetupColumn( "##disasmtableaddr", ImGuiTableColumnFlags_NoHeaderLabel, 0.2F );
  ImGui::TableSetupColumn( "##disasmtableop", ImGuiTableColumnFlags_NoHeaderLabel, 0.8F );
    
  do
  {
    ImGui::TableNextRow();

    if ( mTablePC == mPC )
    {
      ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32( ImGuiCol_Button ) );
    }

    ImGui::TableNextColumn();
    auto label = mManager->mInstance->getTraceHelper()->addressLabel( mTablePC );
    ImGui::Text( label );
    if ( ImGui::IsItemHovered() )
    {
      sprintf( buf, "%s ($%04X)", label, mTablePC );
      ImGui::SetTooltip( buf );
    }

    ImGui::TableNextColumn();
    memset( buf, 0, 6 );
    cpu.disasmOp( buf, (Opcode)ram[mTablePC] );
    ImGui::PushStyleColor( ImGuiCol_Text, opColor );
    ImGui::Text( buf );
    ImGui::PopStyleColor();

    memset( buf, 0, 50 );
    cpu.disasmOpr( ram, (char*)buf, (int&)mTablePC );
    ImGui::SameLine();
    ImGui::Text( buf );
  } while ( --itemCount > 0 );

  ImGui::EndTable();
}

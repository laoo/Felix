#include "pch.hpp"
#include "CPUEditor.hpp"
#include "Manager.hpp"
#include "Core.hpp"
#include "CPU.hpp"
#include "CPUState.hpp"

CPUEditor::CPUEditor()
{
}

void CPUEditor::setManager( Manager* manager )
{
  mManager = manager;
}

bool CPUEditor::enabled()
{
  return mManager && mManager->mInstance && mManager->mDebugger.visualizeCPU;
}

bool CPUEditor::isReadOnly()
{
  if ( !enabled() )
  {
    return true;
  }

  return !mManager->mDebugger.isPaused();
}

void CPUEditor::drawRegister( const char* label, uint8_t reg, char* reg_buf )
{
  snprintf( reg_buf, REG_TXT_LEN, "%02X", reg );
  sprintf( mlabel_buf, "##%s", label );

  ImGui::AlignTextToFramePadding();

  ImGui::Text( label );

  ImGui::SameLine(LABEL_WIDTH);
  ImGui::SetNextItemWidth( ITEM_WIDTH );

  auto inputflag = ImGuiInputTextFlags_CharsHexadecimal | ( isReadOnly() ? ImGuiInputTextFlags_ReadOnly : 0 );

  if ( ImGui::InputText( mlabel_buf, reg_buf, REG_TXT_LEN, inputflag, 0, (void*)label ) )
  {
    auto &state = mManager->mInstance->debugCPU().state();
    uint8_t r = _2CHAR_TO_HEX( reg_buf );

    if ( 0 == strcmp( label, "A") )
    {
      state.a = r;
    }
    else if ( 0 == strcmp( label, "X" ) )
    {
      state.x = r;
    }
    else if ( 0 == strcmp( label, "Y" ) )
    {
      state.y = r;
    }    
  }

  ImGui::SameLine( 2 * ITEM_WIDTH );
  ImGui::Text( BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY( reg ) );

  ImGui::SameLine( 4 * ITEM_WIDTH );
  ImGui::Text( "%d", reg );

  ImGui::SameLine( 5 * ITEM_WIDTH );
  ImGui::Text( "%c", reg );
}

void CPUEditor::drawPS( const char* label, uint16_t ps, char* ps_buf )
{
  snprintf( ps_buf, PS_TXT_LEN, "%04X", ps );
  sprintf( mlabel_buf, "##%s", label );

  ImGui::AlignTextToFramePadding();

  ImGui::Text( label );

  ImGui::SameLine( LABEL_WIDTH );
  ImGui::SetNextItemWidth( ITEM_WIDTH );

  auto inputflag = ImGuiInputTextFlags_CharsHexadecimal | ( isReadOnly() ? ImGuiInputTextFlags_ReadOnly : 0 );

  if ( ImGui::InputText( mlabel_buf, ps_buf, PS_TXT_LEN, inputflag ) )
  {
    auto &state = mManager->mInstance->debugCPU().state();

    int v;
    sscanf( ps_buf, "%04X", &v );

    if ( 0 == strcmp( label, "S" ) )
    {
      state.s = v;
    }
    else if ( 0 == strcmp( label, "PC" ) )
    {
      state.pc = v;
    }
  }
}

void CPUEditor::drawFlag( const char* label, bool enabled, bool* b )
{
  sprintf( mlabel_buf, "##%s", label );
  *b = enabled;
  ImGui::AlignTextToFramePadding();

  ImGui::Text( label );
  ImGui::SameLine();
  ImGui::Checkbox( mlabel_buf, b );
  if ( ImGui::IsItemClicked() )
  {
    auto &state = mManager->mInstance->debugCPU().state();
    int mask;

    if ( 0 == strcmp( label, "N" ) )
    {
      mask = CPUState::bitN;
    }
    else if ( 0 == strcmp( label, "V" ) )
    {
      mask = CPUState::bitV;
    }
    else  if ( 0 == strcmp( label, "D" ) )
    {
      mask = CPUState::bitD;
    }
    else  if ( 0 == strcmp( label, "I" ) )
    {
      mask = CPUState::bitI;
    }
    else  if ( 0 == strcmp( label, "Z" ) )
    {
      mask = CPUState::bitZ;
    }
    else  if ( 0 == strcmp( label, "C" ) )
    {
      mask = CPUState::bitC;
    }
    else
    {
      return;
    }

    auto p = state.getP();
    state.setP( p & mask ? p & ~mask : p | mask );
  }
}

void CPUEditor::drawContents()
{
  if ( !enabled() )
  {
    return;
  }

  auto& state = mManager->mInstance->debugState();

  drawRegister( "A", state.a, mA );
  drawRegister( "X", state.x, mX );
  drawRegister( "Y", state.y, mY );

  drawPS( "PC", state.pc, mPC );
  drawPS( "S", state.s, mS );

  //NVss DIZC
  auto p = state.getP();
  drawFlag( "N", p & CPUState::bitN, &mN );
  ImGui::SameLine(); drawFlag( "V", p & CPUState::bitV, &mV );
  ImGui::SameLine(); drawFlag( "D", p & CPUState::bitD, &mD );
  ImGui::SameLine(); drawFlag( "I", p & CPUState::bitI, &mI );
  ImGui::SameLine(); drawFlag( "Z", p & CPUState::bitZ, &mZ );
  ImGui::SameLine(); drawFlag( "C", p & CPUState::bitC, &mC );

}

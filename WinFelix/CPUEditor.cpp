#include "pch.hpp"
#include "CPUEditor.hpp"
#include "Manager.hpp"
#include "Core.hpp"
#include "CPU.hpp"
#include "CPUState.hpp"

CPUEditor::CPUEditor()
{
}

bool CPUEditor::enabled()
{
  return mManager && mManager->mInstance;
}

bool CPUEditor::isReadOnly()
{
  if ( !enabled() )
  {
    return true;
  }

  return !mManager->mDebugger.isPaused();
}

void CPUEditor::drawRegister( const char* label, uint8_t reg )
{
  std::array<char, 3> regBuf{};
  std::array<char, 10> labelbuf{};

  sprintf( regBuf.data(), "%02x", reg );
  sprintf( labelbuf.data(), "##%s", label );

  ImGui::AlignTextToFramePadding();

  ImGui::Text( label );

  ImGui::SameLine( LABEL_WIDTH );
  ImGui::SetNextItemWidth( ITEM_WIDTH );

  ImGui::BeginDisabled( isReadOnly() );
  if ( ImGui::InputText( labelbuf.data(), regBuf.data(), regBuf.size(), ImGuiInputTextFlags_CharsHexadecimal, 0, (void*)label ) )
  {
    int r;
    std::from_chars( regBuf.data(), regBuf.data() + regBuf.size(), r, 16 );
    auto &state = mManager->mInstance->debugCPU().state();

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
  ImGui::EndDisabled();

  ImGui::SameLine( 2 * ITEM_WIDTH );

  std::array<char, 9> charsBuf{};

  auto tcr = std::to_chars( charsBuf.data(), charsBuf.data() + charsBuf.size(), reg, 2 );
  //to_chars strips leading zeroes
  if ( std::distance( charsBuf.data(), tcr.ptr ) < 8 )
  {
    for ( int i = 7; i >= 0; --i )
    {
      charsBuf[i] = --tcr.ptr >= charsBuf.data() ? *tcr.ptr : '0';
    }
  }

  ImGui::Text( charsBuf.data() );

  ImGui::SameLine( 4 * ITEM_WIDTH );
  ImGui::Text( "%d", reg );

  ImGui::SameLine( 5 * ITEM_WIDTH );
  ImGui::Text( "%c", reg );
}

void CPUEditor::drawPS( const char* label, uint16_t ps )
{
  std::array<char, 5> regBuf{};
  std::array<char, 10> labelbuf{};
  sprintf( regBuf.data(), "%04x", ps );
  sprintf( labelbuf.data(), "##%s", label );

  ImGui::AlignTextToFramePadding();

  ImGui::Text( label );

  ImGui::SameLine( LABEL_WIDTH );
  ImGui::SetNextItemWidth( ITEM_WIDTH );

  ImGui::BeginDisabled( isReadOnly() );
  if ( ImGui::InputText( labelbuf.data(), regBuf.data(), regBuf.size(), ImGuiInputTextFlags_CharsHexadecimal ) )
  {
    int v;
    std::from_chars( regBuf.data(), regBuf.data() + regBuf.size(), v, 16 );

    auto &state = mManager->mInstance->debugCPU().state();

    if ( 0 == strcmp( label, "S" ) )
    {
      state.s = ( v & 0xff ) | 0x0100;
    }
    else if ( 0 == strcmp( label, "PC" ) )
    {
      state.pc = v;
    }
  }
  ImGui::EndDisabled();
}

void CPUEditor::drawFlag( const char* label, bool enabled, bool* b )
{
  std::array<char, 10> labelbuf{};

  sprintf( labelbuf.data(), "##%s", label );
  *b = enabled;
  ImGui::AlignTextToFramePadding();

  ImGui::Text( label );
  ImGui::SameLine();
  ImGui::Checkbox( labelbuf.data(), b );
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

  drawRegister( "A", state.a );
  drawRegister( "X", state.x );
  drawRegister( "Y", state.y );

  drawPS( "PC", state.pc );
  drawPS( "S", state.s );

  //NVss DIZC
  auto p = state.getP();
  ImGui::BeginDisabled( isReadOnly() );
  drawFlag( "N", p & CPUState::bitN, &mN );
  ImGui::SameLine(); drawFlag( "V", p & CPUState::bitV, &mV );
  ImGui::SameLine(); drawFlag( "D", p & CPUState::bitD, &mD );
  ImGui::SameLine(); drawFlag( "I", p & CPUState::bitI, &mI );
  ImGui::SameLine(); drawFlag( "Z", p & CPUState::bitZ, &mZ );
  ImGui::SameLine(); drawFlag( "C", p & CPUState::bitC, &mC );
  ImGui::EndDisabled();
}

void CPUEditor::coreHasBeenReset()
{
};
#include "pch.hpp"
#include "FelixCPUEditor.hpp"
#include "Manager.hpp"
#include "Core.hpp"
#include "CPU.hpp"
#include "CPUState.hpp"

FelixCPUEditor::FelixCPUEditor()
{
}

void FelixCPUEditor::setManager(Manager* manager)
{
    mManager = manager;
}

bool FelixCPUEditor::enabled()
{
    return mManager && mManager->mInstance && mManager->mDebugger.visualizeCPU;
}

bool FelixCPUEditor::isReadOnly()
{
    if (!enabled())
    {
        return true; 
    }

    return !mManager->mDebugger.isPaused();
}

void FelixCPUEditor::drawRegister(const char* label, uint8_t reg, char* reg_buf)
{
    snprintf(reg_buf, REG_TXT_LEN, "%02X", reg);
    
    ImGui::AlignTextToFramePadding();

    ImGui::Text(label);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(ITEM_WIDTH);
    ImGui::InputText("##", reg_buf, REG_TXT_LEN,
        ImGuiInputTextFlags_CharsHexadecimal | 
        ImGuiInputTextFlags_AutoSelectAll |  
        (isReadOnly() ? ImGuiInputTextFlags_ReadOnly : 0)
    );

    ImGui::SameLine(2 * ITEM_WIDTH); 
    ImGui::Text("%d", reg);

    ImGui::SameLine(3 * ITEM_WIDTH); 
    ImGui::Text("%c", reg);
}

void FelixCPUEditor::drawPS(const char* label, uint16_t ps, char* ps_buf)
{
    snprintf(ps_buf, PS_TXT_LEN, "%04X", ps);

    ImGui::AlignTextToFramePadding();

    ImGui::Text(label);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(ITEM_WIDTH);
    ImGui::InputText("##", ps_buf, PS_TXT_LEN,
        ImGuiInputTextFlags_CharsHexadecimal |
        ImGuiInputTextFlags_AutoSelectAll |
        (isReadOnly() ? ImGuiInputTextFlags_ReadOnly : 0)
    );
}

void FelixCPUEditor::drawFlag(const char* label, bool enabled, bool* b)
{
    *b = enabled;
    ImGui::AlignTextToFramePadding();

    ImGui::Text(label);
    ImGui::SameLine();
    ImGui::Checkbox("##", b);
}

void FelixCPUEditor::drawContents()
{
    if (!enabled())
    {
        return;
    }

    auto& state = mManager->mInstance->debugState();

    drawRegister("A", state.a, mA);
    drawRegister("X", state.x, mX);
    drawRegister("Y", state.y, mY);

    drawPS("P", state.pc, mP);
    drawPS("S", state.s, mS);

    //NVss DIZC
    auto p = state.getP();
    drawFlag("N", p & CPUState::bitN, &mN);
    ImGui::SameLine(); drawFlag("V", p & CPUState::bitV, &mV);
    ImGui::SameLine(); drawFlag("D", p & CPUState::bitD, &mD);
    ImGui::SameLine(); drawFlag("I", p & CPUState::bitI, &mI);
    ImGui::SameLine(); drawFlag("Z", p & CPUState::bitZ, &mZ);
    ImGui::SameLine(); drawFlag("C", p & CPUState::bitC, &mC);

}
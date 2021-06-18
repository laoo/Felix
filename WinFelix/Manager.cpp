#include "pch.hpp"
#include "Manager.hpp"
#include "imgui.h"

Manager::Manager() : mEmulationRunning{ true }, mHorizontalView{ true }
{
}

void Manager::drawGui( int left, int top, int right, int bottom )
{
  ImGuiIO & io = ImGui::GetIO();

  bool hovered = io.MousePos.x > left && io.MousePos.y > top && io.MousePos.x < right && io.MousePos.y < bottom;

  if ( hovered )
  {
    ImGui::PushStyleVar( ImGuiStyleVar_Alpha, std::clamp( ( 100.0f - io.MousePos.y ) / 100.f, 0.0f, 1.0f ) );
    if ( ImGui::BeginMainMenuBar() )
    {
      ImGui::PushStyleVar( ImGuiStyleVar_Alpha, 1.0f );
      if ( ImGui::BeginMenu( "File" ) )
      {
        if ( ImGui::MenuItem( "Exit", "Alt+F4" ) )
        {
          mEmulationRunning = false;
        }
        ImGui::EndMenu();
      }
      if ( ImGui::BeginMenu( "View" ) )
      {
        ImGui::Checkbox( "Horizontal", &mHorizontalView );
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
      ImGui::PopStyleVar();
    }
    ImGui::PopStyleVar();
  }

  //ImGui::ShowDemoWindow();
}

void Manager::horizontalView( bool horizontal )
{
  mHorizontalView = horizontal;
}

bool Manager::doRun() const
{
  return mEmulationRunning;
}

bool Manager::horizontalView() const
{
  return mHorizontalView;
}


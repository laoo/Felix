#include "pch.hpp"
#include "Config.hpp"
#include "imgui.h"

Config::Config() : mEmulationRunning{ true }, mHorizontalView{ true }
{
}

void Config::drawGui( int left, int top, int right, int bottom )
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

void Config::horizontalView( bool horizontal )
{
  mHorizontalView = horizontal;
}

bool Config::doRun() const
{
  return mEmulationRunning;
}

bool Config::horizontalView() const
{
  return mHorizontalView;
}


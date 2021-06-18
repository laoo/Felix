#include "pch.hpp"
#include "Manager.hpp"
#include "imgui.h"

Manager::Manager() : mEmulationRunning{ true }, mHorizontalView{ true }, mIntputSources{}
{
  mIntputSources[0] = std::make_shared<InputSource>();
  mIntputSources[1] = std::make_shared<InputSource>();
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

void Manager::processKeys()
{
  std::array<uint8_t, 256> keys;
  if ( !GetKeyboardState( keys.data() ) )
    return;

  mIntputSources[0]->left = keys['A'] & 0x80;
  mIntputSources[0]->up = keys['W'] & 0x80;
  mIntputSources[0]->right = keys['D'] & 0x80;
  mIntputSources[0]->down = keys['S'] & 0x80;
  mIntputSources[0]->opt1 = keys['1'] & 0x80;
  mIntputSources[0]->pause = keys['2'] & 0x80;
  mIntputSources[0]->opt2 = keys['3'] & 0x80;
  mIntputSources[0]->a = keys[VK_LCONTROL] & 0x80;
  mIntputSources[0]->b = keys[VK_LSHIFT] & 0x80;

  mIntputSources[1]->left = keys[VK_LEFT] & 0x80;
  mIntputSources[1]->up = keys[VK_UP] & 0x80;
  mIntputSources[1]->right = keys[VK_RIGHT] & 0x80;
  mIntputSources[1]->down = keys[VK_DOWN] & 0x80;
  mIntputSources[1]->opt1 = keys[VK_DELETE] & 0x80;
  mIntputSources[1]->pause = keys[VK_END] & 0x80;
  mIntputSources[1]->opt2 = keys[VK_NEXT] & 0x80;
  mIntputSources[1]->a = keys[VK_RCONTROL] & 0x80;
  mIntputSources[1]->b = keys[VK_RSHIFT] & 0x80;
}

std::shared_ptr<IInputSource> Manager::getInputSource( int instance )
{
  if ( instance >= 0 && instance < mIntputSources.size() )
    return mIntputSources[instance];
  else
    return {};
}

Manager::InputSource::InputSource() : KeyInput{}
{
}

KeyInput Manager::InputSource::getInput() const
{
  return *this;
}

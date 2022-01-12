#pragma once

#include "IInputSource.hpp"

class Manager;
struct ImGuiIO;
struct DebugWindow;

namespace ImGui
{
class FileBrowser;
}

class UI
{
public:
  UI( Manager& manager );
  ~UI();

  void drawGui( int left, int top, int right, int bottom );

private:
  bool mainMenu( ImGuiIO& io );
  void drawDebugWindows( ImGuiIO& io );
  void configureKeyWindow( std::optional<KeyInput::Key>& keyToConfigure );
  void imagePropertiesWindow( bool init );
  void renderBoard( DebugWindow& win );

private:
  Manager& mManager;
  bool mOpenMenu;
  std::unique_ptr<ImGui::FileBrowser> mFileBrowser;
};

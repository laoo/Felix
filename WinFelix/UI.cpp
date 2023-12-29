#include "pch.hpp"
#include "UI.hpp"
#include "imgui.h"
#include "IInputSource.hpp"
#include "Manager.hpp"
#include "KeyNames.hpp"
#include "UserInput.hpp"
#include "ConfigProvider.hpp"
#include <imfilebrowser.h>
#include "WinAudioOut.hpp"
#include "Core.hpp"
#include "CPU.hpp"
#include "SysConfig.hpp"

UI::UI( Manager& manager ) :
  mManager{ manager },
  mOpenMenu{},
  mFileBrowser{ std::make_unique<ImGui::FileBrowser>(ImGuiFileBrowserFlags_EnterNewFilename) }
{
}

UI::~UI()
{
}

void UI::drawGui( int left, int top, int right, int bottom )
{
  ImGuiIO& io = ImGui::GetIO();

  bool hovered = io.MousePos.x > left && io.MousePos.y > top && io.MousePos.x < right&& io.MousePos.y < bottom;

  if ( hovered || mOpenMenu )
  {
    mOpenMenu = mainMenu( io );
  }

  if ( mManager.mExtendedRenderer )
    drawDebugWindows( io );
}

bool UI::mainMenu( ImGuiIO& io )
{
  enum class FileBrowserAction
  {
    NONE,
    OPEN_CARTRIDGE,
    OPEN_BOOTROM,
    OPEN_WAVE,
    OPEN_VGM
  };

  enum class ModalWindow
  {
    NONE,
    PROPERTIES
  };

  static ModalWindow modalWindow = ModalWindow::NONE;
  static FileBrowserAction fileBrowserAction = FileBrowserAction::NONE;
  static std::optional<KeyInput::Key> keyToConfigure;

  auto configureKeyItem = [&] ( char const* name, KeyInput::Key k )
  {
    ImGui::Text( name );
    ImGui::SameLine( 60 );

    if ( ImGui::Button( keyName( mManager.userInput().getVirtualCode( k ) ), ImVec2( 100, 0 ) ) )
    {
      keyToConfigure = k;
      ImGui::OpenPopup( "Configure Key" );
    }
  };

  auto sysConfig = gConfigProvider.sysConfig();

  bool openMenu = false;
  bool pauseRunIssued = false;
  bool stepInIssued = false;
  bool stepOverIssued = false;
  bool stepOutIssued = false;
  bool resetIssued = false;
  bool debugMode = mManager.mDebugger.isDebugMode();

  if ( ImGui::IsKeyPressed( ImGuiKey_F3 ) )
  {
    resetIssued = true;
  }

  if ( ImGui::IsKeyPressed( ImGuiKey_F4 ) && mManager.mExtendedRenderer )
  {
    debugMode = !debugMode;
  }

  if ( ImGui::IsKeyPressed( ImGuiKey_F5 ) )
  {
    pauseRunIssued = true;
  }
  else if ( ImGui::IsKeyPressed( ImGuiKey_F6 ) )
  {
    stepInIssued = true;
  }
  else if ( ImGui::IsKeyPressed( ImGuiKey_F7 ) )
  {
    stepOverIssued = true;
  }
  else if ( ImGui::IsKeyPressed( ImGuiKey_F8 ) )
  {
    stepOutIssued = true;
  }


  ImGui::PushStyleVar( ImGuiStyleVar_Alpha, mOpenMenu ? 1.0f : std::clamp( ( 100.0f - io.MousePos.y ) / 100.f, 0.0f, 1.0f ) );
  if ( ImGui::BeginMainMenuBar() )
  {
    ImGui::PushStyleVar( ImGuiStyleVar_Alpha, 1.0f );
    if ( ImGui::BeginMenu( "File" ) )
    {
      openMenu = true;
      if ( ImGui::MenuItem( "Open", "Ctrl+O"))
      {
        mFileBrowser->SetTitle( "Open Cartridge image file" );
        mFileBrowser->SetTypeFilters( { ".lnx", ".lyx", ".o" } );
        if ( auto openPath = gConfigProvider.sysConfig()->lastOpenDirectory; !openPath.empty() )
        {
          mFileBrowser->SetPwd( gConfigProvider.sysConfig()->lastOpenDirectory );
        }
        mFileBrowser->Open();
        fileBrowserAction = FileBrowserAction::OPEN_CARTRIDGE;
      }
      if (ImGui::MenuItem("Re-Open", "Ctrl+R"))
      {
          mManager.mDoReset = true;
      }
      if ( ImGui::MenuItem( "Exit", "Alt+F4" ) )
      {
        mManager.quit();
      }
      ImGui::EndMenu();
    }
    ImGui::BeginDisabled( !(bool)mManager.mImageProperties );
    if ( ImGui::BeginMenu( "Cartridge" ) )
    {
      openMenu = true;
      if ( ImGui::MenuItem( "Properties", "Ctrl+P", nullptr ) )
      {
        modalWindow = ModalWindow::PROPERTIES;
      }
      ImGui::EndMenu();
    }
    ImGui::EndDisabled();
    if ( ImGui::BeginMenu( "Audio" ) )
    {
      openMenu = true;
      bool mute = mManager.mAudioOut->mute();
      if ( ImGui::MenuItem( "Mute", "Ctrl+M", &mute ) )
      {
        mManager.mAudioOut->mute( mute );
      }
      bool wavOut = mManager.mAudioOut->isWavOut();
      if ( ImGui::MenuItem( "Wav Out", nullptr, &wavOut ) )
      {
        if ( wavOut )
        {
          mFileBrowser->SetTitle( "Save audio to wav file" );
          mFileBrowser->SetTypeFilters( { ".wav", ".*" } );
          mFileBrowser->Open();
          fileBrowserAction = FileBrowserAction::OPEN_WAVE;
        }
        else
        {
          mManager.mAudioOut->setWavOut( std::filesystem::path{} );
        }
      }
      ImGui::BeginDisabled( !( bool )mManager.mInstance );
      bool vgmOut = mManager.mInstance ? mManager.mInstance->isVGMWriter() : false;
      if ( ImGui::MenuItem( "VGM Out", nullptr, &vgmOut ) )
      {
        if ( vgmOut )
        {
          mFileBrowser->SetTitle( "Save VGM 1.72" );
          mFileBrowser->SetTypeFilters( { ".vgm", ".*" } );
          mFileBrowser->Open();
          fileBrowserAction = FileBrowserAction::OPEN_VGM;
        }
        else
        {
          mManager.mInstance->setVGMWriter( {} );
        }
      }
      ImGui::EndDisabled();
      ImGui::EndMenu();
    }
    if ( mManager.mExtendedRenderer )
    {
      ImGui::BeginDisabled( !(bool)mManager.mInstance );
      if ( ImGui::BeginMenu( "Debug" ) )
      {
        openMenu = true;

        if ( ImGui::MenuItem( "Reset", "F3" ) )
        {
          resetIssued = true;
        }

        ImGui::MenuItem( "Debug Mode", "F4", &debugMode );

        if ( ImGui::MenuItem( mManager.mDebugger.isPaused() ? "Run" : "Break", "F5" ) )
        {
          pauseRunIssued = true;
        }
        if ( ImGui::MenuItem( "Step In", "F6" ) )
        {
          stepInIssued = true;
        }
        if ( ImGui::MenuItem( "Step Over", "F7" ) )
        {
          stepOverIssued = true;
        }
        if ( ImGui::MenuItem( "Step Out", "F8" ) )
        {
          stepOutIssued = true;
        }
        if ( ImGui::BeginMenu( "Debug Windows" ) )
        {
          bool cpuWindow = mManager.mDebugger.visualizeCPU;
          bool watchWindow = mManager.mDebugger.visualizeWatch;
          bool breakpointWindow = mManager.mDebugger.visualizeBreakpoint;
          bool memoryWindow = mManager.mDebugger.visualizeMemory;
          bool disasmWindow = mManager.mDebugger.visualizeDisasm;
          bool historyWindow = mManager.mDebugger.isHistoryVisualized();
          if ( ImGui::MenuItem( "CPU Window", "Ctrl+C", &cpuWindow ) )
          {
            mManager.mDebugger.visualizeCPU = cpuWindow;
          }
          if ( ImGui::MenuItem( "Disassembly Window", "Ctrl+D", &disasmWindow ) )
          {
            mManager.mDebugger.visualizeDisasm = disasmWindow;
          }
          if ( ImGui::MenuItem( "Memory Window", "Ctrl+N", &memoryWindow ) )
          {
            mManager.mDebugger.visualizeMemory = memoryWindow;
          }
          if ( ImGui::MenuItem( "Watch Window", "Ctrl+W", &watchWindow ) )
          {
            mManager.mDebugger.visualizeWatch = watchWindow;
          }
          if ( ImGui::MenuItem( "Breakpoint Window", "Ctrl+B", &breakpointWindow ) )
          {
            mManager.mDebugger.visualizeBreakpoint = breakpointWindow;
          }
          if ( ImGui::MenuItem( "History Window", "Ctrl+H", &historyWindow ) )
          {
            if ( historyWindow )
            {
              mManager.mInstance->debugCPU().enableHistory( mManager.mDebugger.historyVisualizer().columns, mManager.mDebugger.historyVisualizer().rows );
              mManager.mDebugger.visualizeHistory( true );
            }
            else
            {
              mManager.mInstance->debugCPU().disableHistory();
              mManager.mDebugger.visualizeHistory( false );
            }
          }
          ImGui::BeginDisabled( !mManager.mDebugger.isDebugMode() );
          if ( ImGui::MenuItem( "New Screen View", "Ctrl+S" ) )
          {
            mManager.mDebugger.newScreenView();
          }
          ImGui::EndDisabled();
          ImGui::EndMenu();
        }
        if ( ImGui::BeginMenu( "Options" ) )
        {
          bool breakOnBrk = mManager.mDebugger.isBreakOnBrk();
          bool debugModeOnBreak = mManager.mDebugger.debugModeOnBreak();
          bool normalModeOnRun = mManager.mDebugger.normalModeOnRun();

          if ( ImGui::MenuItem( "Break on BRK", nullptr, &breakOnBrk ) )
          {
            mManager.mDebugger.breakOnBrk( breakOnBrk );
            mManager.mInstance->debugCPU().breakOnBrk( breakOnBrk );
          }
          if ( ImGui::MenuItem( "Debug mode on break", nullptr, &debugModeOnBreak ) )
          {
            mManager.mDebugger.debugModeOnBreak( debugModeOnBreak );
          }
          if ( ImGui::MenuItem( "Normal mode on run", nullptr, &normalModeOnRun ) )
          {
            mManager.mDebugger.normalModeOnRun( normalModeOnRun );
          }
          ImGui::EndMenu();
        }
        ImGui::EndMenu();
      }
      ImGui::EndDisabled();
    }
    if ( ImGui::BeginMenu( "Options" ) )
    {
      openMenu = true;
      if ( ImGui::BeginMenu( "Input Configuration" ) )
      {
        configureKeyItem( "Left", KeyInput::LEFT );
        configureKeyItem( "Right", KeyInput::RIGHT );
        configureKeyItem( "Up", KeyInput::UP );
        configureKeyItem( "Down", KeyInput::DOWN );
        configureKeyItem( "A", KeyInput::OUTER );
        configureKeyItem( "B", KeyInput::INNER );
        configureKeyItem( "Opt1", KeyInput::OPTION1 );
        configureKeyItem( "Pause", KeyInput::PAUSE );
        configureKeyItem( "Opt2", KeyInput::OPTION2 );

        configureKeyWindow( keyToConfigure );

        ImGui::EndMenu();
      }

      if ( ImGui::BeginMenu( "Boot ROM" ) )
      {
        bool externalSelectEnabled = !sysConfig->bootROM.path.empty();
        if ( ImGui::BeginMenu( "Use external ROM", externalSelectEnabled ) )
        {
          if ( ImGui::Checkbox( "Enabled", &sysConfig->bootROM.useExternal ) )
          {
            mManager.mDoReset = true;
          }
          if ( ImGui::MenuItem( "Clear boot ROM path" ) )
          {
            sysConfig->bootROM.path.clear();
            if ( sysConfig->bootROM.useExternal )
            {
              sysConfig->bootROM.useExternal = false;
              mManager.mDoReset = true;
            }
          }
          ImGui::EndMenu();
        }
        if ( ImGui::MenuItem( "Select image" ) )
        {
          mFileBrowser->SetTitle( "Open boot ROM image file" );
          mFileBrowser->SetTypeFilters( { ".img", ".*" } );
          mFileBrowser->Open();
          fileBrowserAction = FileBrowserAction::OPEN_BOOTROM;
        }
        ImGui::EndMenu();
      }

      ImGui::Checkbox( "Single emulator instance", &sysConfig->singleInstance );
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
    ImGui::PopStyleVar();
  }
  ImGui::PopStyleVar();

  if ( io.KeyCtrl )
  {
      if (ImGui::IsKeyPressed(ImGuiKey_R))
      {     
          mManager.mDoReset = true;
      }
     if (ImGui::IsKeyPressed(ImGuiKey_O))
     {
        mFileBrowser->SetTitle("Open Cartridge image file");
        mFileBrowser->SetTypeFilters({ ".lnx", ".lyx", ".o" });
        if (auto openPath = gConfigProvider.sysConfig()->lastOpenDirectory; !openPath.empty())
        {
            mFileBrowser->SetPwd(gConfigProvider.sysConfig()->lastOpenDirectory);
        }
        mFileBrowser->SetInputName(mManager.mArg.filename().generic_string());

        mFileBrowser->Open();
        fileBrowserAction = FileBrowserAction::OPEN_CARTRIDGE;
     }
    if ( ImGui::IsKeyPressed( ImGuiKey_P ) )
    {
      modalWindow = ModalWindow::PROPERTIES;
    }
    if ( ImGui::IsKeyPressed( ImGuiKey_C ) )
    {
      mManager.mDebugger.visualizeCPU = !mManager.mDebugger.visualizeCPU;
    }
    if ( ImGui::IsKeyPressed( ImGuiKey_D ) )
    {
      mManager.mDebugger.visualizeDisasm = !mManager.mDebugger.visualizeDisasm;
    }
    if ( ImGui::IsKeyPressed( ImGuiKey_N ) )
    {
      mManager.mDebugger.visualizeMemory = !mManager.mDebugger.visualizeMemory;
    }
    if ( ImGui::IsKeyPressed( ImGuiKey_W ) )
    {
      mManager.mDebugger.visualizeWatch = !mManager.mDebugger.visualizeWatch;
    }
    if ( ImGui::IsKeyPressed( ImGuiKey_B ) )
    {
      mManager.mDebugger.visualizeBreakpoint = !mManager.mDebugger.visualizeBreakpoint;
    }
    if ( ImGui::IsKeyPressed( ImGuiKey_H ) )
    {
      bool historyWindow = !mManager.mDebugger.isHistoryVisualized();
      if ( historyWindow )
      {
        mManager.mInstance->debugCPU().enableHistory( mManager.mDebugger.historyVisualizer().columns, mManager.mDebugger.historyVisualizer().rows );
        mManager.mDebugger.visualizeHistory( true );
      }
      else
      {
        mManager.mInstance->debugCPU().disableHistory();
        mManager.mDebugger.visualizeHistory( false );
      }
    }
    if ( ImGui::IsKeyPressed( ImGuiKey_S ) && mManager.mDebugger.isDebugMode() )
    {
      mManager.mDebugger.newScreenView();
    }
    if ( ImGui::IsKeyPressed( ImGuiKey_M ) )
    {
      mManager.mAudioOut->mute( !mManager.mAudioOut->mute() );
    }
  }

  mManager.mDebugger.debugMode( debugMode );

  if ( resetIssued )
  {
    mManager.reset();
    mManager.mDebugger( RunMode::PAUSE );
  }
  if ( stepOutIssued )
  {
    mManager.mDebugger( RunMode::STEP_OUT );
  }
  if ( stepOverIssued )
  {
    mManager.mDebugger( RunMode::STEP_OVER );
  }
  if ( stepInIssued )
  {
    mManager.mDebugger( RunMode::STEP_IN );
  }
  else if ( pauseRunIssued )
  {
    mManager.mDebugger.togglePause();
  }

  switch ( modalWindow )
  {
  case ModalWindow::PROPERTIES:
    ImGui::OpenPopup( "Image properties" );
    break;
  default:
    break;
  }

  imagePropertiesWindow( modalWindow == ModalWindow::PROPERTIES );

  modalWindow = ModalWindow::NONE;


  mFileBrowser->Display();
  if ( mFileBrowser->HasSelected() )
  {
    using enum FileBrowserAction;
    switch ( fileBrowserAction )
    {
    case OPEN_CARTRIDGE:
      mManager.mArg = mFileBrowser->GetSelected();
      if ( auto parent = mManager.mArg.parent_path(); !parent.empty() )
      {
        gConfigProvider.sysConfig()->lastOpenDirectory = mManager.mArg.parent_path();
      }
      mManager.mDoReset = true;
      break;
    case OPEN_BOOTROM:
      sysConfig->bootROM.path = mFileBrowser->GetSelected();
      break;
    case OPEN_WAVE:
      mManager.mAudioOut->setWavOut( mFileBrowser->GetSelected() );
      break;
    case OPEN_VGM:
      mManager.mInstance->setVGMWriter( mFileBrowser->GetSelected() );
      break;
    }
    mFileBrowser->ClearSelected();
    fileBrowserAction = NONE;
  }

  return openMenu;
}

void UI::drawDebugWindows( ImGuiIO& io )
{
  assert( mManager.mExtendedRenderer );

  std::unique_lock<std::mutex> l = mManager.mDebugger.lockMutex();

  auto historyRendering = mManager.renderHistoryWindow();
  bool debugMode = mManager.mDebugger.isDebugMode();

  if ( debugMode )
  {
    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2{ 2.0f, 2.0f } );

    if ( mManager.mDebugger.visualizeCPU )
    {
      ImGui::Begin( "CPU", &mManager.mDebugger.visualizeCPU, ImGuiWindowFlags_AlwaysAutoResize );
      mManager.mDebugWindows.cpuEditor.drawContents();
      ImGui::End();
    }

    if ( mManager.mDebugger.visualizeMemory )
    {
      ImGui::Begin( "Memory", &mManager.mDebugger.visualizeMemory, ImGuiWindowFlags_None );
      mManager.mDebugWindows.memoryEditor.drawContents();
      ImGui::End();
    }

    if ( mManager.mDebugger.visualizeWatch )
    {
      ImGui::Begin( "Watch", &mManager.mDebugger.visualizeWatch, ImGuiWindowFlags_None );
      mManager.mDebugWindows.watchEditor.drawContents();
      ImGui::End();
    }

    if ( mManager.mDebugger.visualizeBreakpoint )
    {
      ImGui::Begin( "Breakpoint", &mManager.mDebugger.visualizeBreakpoint, ImGuiWindowFlags_None );
      mManager.mDebugWindows.breakpointEditor.drawContents();
      ImGui::End();
    }

    if ( mManager.mDebugger.visualizeDisasm )
    {
      ImGui::Begin( "Disassembly", &mManager.mDebugger.visualizeDisasm, ImGuiWindowFlags_None );
      mManager.mDebugWindows.disasmEditor.drawContents();
      ImGui::End();
    }

    if ( historyRendering.enabled )
    {
      ImGui::Begin( "History", &historyRendering.enabled, ImGuiWindowFlags_AlwaysAutoResize );
      ImGui::Image( historyRendering.window, ImVec2{ historyRendering.width, historyRendering.height } );
      ImGui::End();
    }

    if ( auto mainScreenView = mManager.mDebugWindows.mainScreenView )
    {
      static const float xpad = 4.0f;
      static const float ypad = 4.0f + 19.0f;
      ImGui::PushStyleVar( ImGuiStyleVar_WindowMinSize, ImVec2{ SCREEN_WIDTH + xpad, SCREEN_HEIGHT + ypad } );

      ImGui::Begin( "Rendering", &debugMode, ImGuiWindowFlags_NoCollapse );
      auto size = ImGui::GetWindowSize();
      size.x = std::max( 0.0f, size.x - xpad );
      size.y = std::max( 0.0f, size.y - ypad );
      if ( auto tex = mainScreenView->getTexture() )
      {
        ImGui::Image( tex, size );
      }
      ImGui::End();
      ImGui::PopStyleVar();
      mainScreenView->resize( (int)size.x, (int)size.y );
    }

    std::vector<int> removedIds;
    for ( auto& sv : mManager.mDebugger.screenViews() )
    {
      static const float xpad = 4.0f;
      static const float ypad = ( 4.0f + 19.0f ) * 2;
      ImGui::PushStyleVar( ImGuiStyleVar_WindowMinSize, ImVec2{ SCREEN_WIDTH + xpad, SCREEN_HEIGHT + ypad } );

      char buf[64];
      std::sprintf( buf, "Screen View %d", sv.id );
      bool open = true;
      ImGui::Begin( buf, &open, 0 );
      if ( !open )
        removedIds.push_back( sv.id );
      ImGui::SetNextItemWidth( 80 );
      ImGui::Combo( "##sv", (int*)&sv.type, "dispadr\0vidbase\0collbas\0custom\0" );
      ImGui::SameLine();
      std::span<uint8_t const> data{};
      std::span<uint8_t const> palette{};

      switch ( sv.type )
      {
      case ScreenViewType::DISPADR:
        ImGui::BeginDisabled();
        if ( mManager.mInstance )
        {
          uint16_t addr = mManager.mInstance->debugDispAdr();
          std::sprintf( buf, "%04x", addr );
          data = std::span<uint8_t const>{ mManager.mInstance->debugRAM() + addr, SCREEN_WIDTH * SCREEN_HEIGHT / 2 };
          if ( !sv.safePalette )
            palette = mManager.mInstance->debugPalette();
        }
        break;
      case ScreenViewType::VIDBAS:
        ImGui::BeginDisabled();
        if ( mManager.mInstance )
        {
          uint16_t addr = mManager.mInstance->debugVidBas();
          std::sprintf( buf, "%04x", addr );
          data = std::span<uint8_t const>{ mManager.mInstance->debugRAM() + addr, SCREEN_WIDTH * SCREEN_HEIGHT / 2 };
          if ( !sv.safePalette )
            palette = mManager.mInstance->debugPalette();
        }
        break;
      case ScreenViewType::COLLBAS:
        ImGui::BeginDisabled();
        if ( mManager.mInstance )
        {
          uint16_t addr = mManager.mInstance->debugCollBas();
          std::sprintf( buf, "%04x", addr );
          data = std::span<uint8_t const>{ mManager.mInstance->debugRAM() + addr, SCREEN_WIDTH * SCREEN_HEIGHT / 2 };
          if ( !sv.safePalette )
            palette = mManager.mInstance->debugPalette();
        }
        break;
      default:  //ScreenViewType::CUSTOM:
        ImGui::BeginDisabled( false );
        if ( mManager.mInstance )
        {
          uint16_t addr = sv.customAddress;
          std::sprintf( buf, "%04x", sv.customAddress );
          data = std::span<uint8_t const>{ mManager.mInstance->debugRAM() + sv.customAddress, SCREEN_WIDTH * SCREEN_HEIGHT / 2 };
          if ( !sv.safePalette )
            palette = mManager.mInstance->debugPalette();
        }
        break;
      }
      ImGui::SetNextItemWidth( 40 );
      if ( ImGui::InputTextWithHint( "##ha", "hex addr", buf, 5, ImGuiInputTextFlags_CharsHexadecimal ) )
      {
        int hex;
        std::from_chars( &buf[0], &buf[5], hex, 16 );
        hex = std::min( hex, 0xe000 );
        sv.customAddress = (uint16_t)( hex & 0b1111111111111100 );
      }
      ImGui::EndDisabled();
      ImGui::SameLine();
      ImGui::Checkbox( "safe palette", &sv.safePalette );
      auto size = ImGui::GetWindowSize();
      size.x = std::max( 0.0f, size.x - xpad );
      size.y = std::max( 0.0f, size.y - ypad );

      auto it = std::ranges::find( mManager.mDebugWindows.customScreenViews, sv.id, [] ( auto const& p ) { return p.first; } );
      if ( it != mManager.mDebugWindows.customScreenViews.cend() )
      {
        if ( auto tex = it->second->render( data, palette ) )
        {
          ImGui::Image( tex, size );
        }
        it->second->resize( (int)size.x, (int)size.y );
      }

      ImGui::End();
      ImGui::PopStyleVar();
    }

    for ( int id : removedIds )
    {
      mManager.mDebugger.delScreenView( id );
    }

    mManager.mDebugger.debugMode( debugMode );
    ImGui::PopStyleVar();


    if ( ImGui::BeginPopupContextVoid() )
    {
      ImGui::Checkbox( "CPU Window", &mManager.mDebugger.visualizeCPU );
      ImGui::Checkbox( "Disassembly Window", &mManager.mDebugger.visualizeDisasm );
      ImGui::Checkbox( "Memory Window", &mManager.mDebugger.visualizeMemory );
      if ( ImGui::Checkbox( "History Window", &historyRendering.enabled ) )
      {
        if ( historyRendering.enabled )
        {
          mManager.mInstance->debugCPU().enableHistory( mManager.mDebugger.historyVisualizer().columns, mManager.mDebugger.historyVisualizer().rows );
        }
        else
        {
          mManager.mInstance->debugCPU().disableHistory();
        }
      }
      if ( ImGui::Selectable( "New Screen View" ) )
      {
        mManager.mDebugger.newScreenView();
      }
      ImGui::EndPopup();
    }

    mManager.mDebugger.visualizeHistory( historyRendering.enabled );
  }
}

void UI::configureKeyWindow( std::optional<KeyInput::Key>& keyToConfigure )
{
  if ( ImGui::BeginPopupModal( "Configure Key", NULL, ImGuiWindowFlags_AlwaysAutoResize ) )
  {
    if ( ImGui::BeginTable( "table", 3 ) )
    {
      ImGui::TableSetupColumn( "1", ImGuiTableColumnFlags_WidthFixed );
      ImGui::TableSetupColumn( "2", ImGuiTableColumnFlags_WidthFixed, 100.0f );
      ImGui::TableSetupColumn( "3", ImGuiTableColumnFlags_WidthFixed );

      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text( "Press key" );
      ImGui::TableNextRow( ImGuiTableRowFlags_None, 30.0f );
      ImGui::TableNextColumn();
      ImGui::TableNextColumn();
      static int code = 0;
      if ( code == 0 )
      {
        code = mManager.userInput().getVirtualCode( *keyToConfigure );
      }
      if ( auto c = mManager.userInput().firstKeyPressed() )
      {
        code = c;
      }
      ImGui::Text( keyName( code ) );
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      if ( ImGui::Button( "OK", ImVec2( 60, 0 ) ) )
      {
        mManager.userInput().updateMapping( *keyToConfigure, code );
        keyToConfigure = std::nullopt;
        code = 0;
        ImGui::CloseCurrentPopup();
      }
      ImGui::TableNextColumn();
      ImGui::TableNextColumn();
      ImGui::SetItemDefaultFocus();
      if ( ImGui::Button( "Cancel", ImVec2( 60, 0 ) ) )
      {
        keyToConfigure = std::nullopt;
        code = 0;
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndTable();
    }
    ImGui::EndPopup();
  }
}

void UI::imagePropertiesWindow( bool init )
{
  if ( ImGui::BeginPopupModal( "Image properties", NULL, ImGuiWindowFlags_AlwaysAutoResize ) )
  {
    static int rotation;
    static ImageProperties::EEPROM eeprom;
    if ( init )
    {
      rotation = (int)mManager.mImageProperties->getRotation();
      eeprom = mManager.mImageProperties->getEEPROM();
    }

    auto isChanged = [&]
    {
      if ( rotation != (int)mManager.mImageProperties->getRotation() )
        return true;
      if ( eeprom.bits != mManager.mImageProperties->getEEPROM().bits )
        return true;

      return false;
    };

    auto cartName = mManager.mImageProperties->getCartridgeName();
    auto manufName = mManager.mImageProperties->getMamufacturerName();
    auto const& bankProps = mManager.mImageProperties->getBankProps();
    ImGui::TextUnformatted( "Cart Name:" );
    ImGui::SameLine();
    ImGui::TextUnformatted( cartName.data(), cartName.data() + cartName.size() );
    ImGui::TextUnformatted( "Manufacturer:" );
    ImGui::SameLine();
    ImGui::TextUnformatted( manufName.data(), manufName.data() + manufName.size() );
    ImGui::TextUnformatted( "Size:" );
    ImGui::SameLine();
    if ( bankProps[2].numberOfPages > 0 )
    {
      ImGui::Text( "( %d + %d ) * %d B = %d B", bankProps[0].numberOfPages, bankProps[2].numberOfPages, bankProps[0].pageSize, ( bankProps[0].numberOfPages + bankProps[2].numberOfPages ) * bankProps[0].pageSize );
    }
    else
    {
      ImGui::Text( "%d * %d B = %d B", bankProps[0].numberOfPages, bankProps[0].pageSize, bankProps[0].numberOfPages * bankProps[0].pageSize );
    }
    if ( bankProps[1].pageSize * bankProps[1].numberOfPages > 0 )
    {
      ImGui::TextUnformatted( "Aux Size:" );
      ImGui::SameLine();
      if ( bankProps[3].numberOfPages > 0 )
      {
        ImGui::Text( "( %d + %d ) * %d B = %d B", bankProps[1].numberOfPages, bankProps[3].numberOfPages, bankProps[1].pageSize, ( bankProps[1].numberOfPages + bankProps[3].numberOfPages ) * bankProps[1].pageSize );
      }
      else
      {
        ImGui::Text( "%d * %d B = %d B", bankProps[1].numberOfPages, bankProps[1].pageSize, bankProps[1].numberOfPages * bankProps[1].pageSize );
      }
    }
    ImGui::TextUnformatted( "AUDIn Used?:" );
    ImGui::SameLine();
    ImGui::TextUnformatted( mManager.mImageProperties->getAUDInUsed() ? "Yes" : "No" );
    ImGui::TextUnformatted( "Rotation:" );
    ImGui::SameLine();
    ImGui::SetNextItemWidth( 80 );
    ImGui::Combo( "##r", &rotation, "Normal\0Left\0Right\0" );
    ImGui::TextUnformatted( "EEPROM:" );
    ImGui::SameLine();
    int eepromType = eeprom.type();
    ImGui::SetNextItemWidth( 80 );
    if ( ImGui::Combo( "##", &eepromType, ImageProperties::EEPROM::NAMES.data(), ImageProperties::EEPROM::TYPE_COUNT ) )
    {
      eeprom.setType( eepromType );
    }
    if ( eeprom.type() != 0 )
    {
      ImGui::TextUnformatted( "EEPROM bitness:" );
      ImGui::SameLine();
      int bitness = eeprom.is16Bit() ? 1 : 0;
      if ( ImGui::RadioButton( "8-bit", &bitness, 0 ) )
      {
        eeprom.set16bit( false );
      }
      ImGui::SameLine();
      if ( ImGui::RadioButton( "16-bit", &bitness, 1 ) )
      {
        eeprom.set16bit( true );
      }
    }
    ImGui::TextUnformatted( "SD Card support:" );
    ImGui::SameLine();
    int sd = eeprom.sd() ? 1 : 0;
    if ( ImGui::RadioButton( "No", &sd, 0 ) )
    {
      eeprom.setSD( false );
    }
    ImGui::SameLine();
    if ( ImGui::RadioButton( "Yes", &sd, 1 ) )
    {
      eeprom.setSD( true );
    }
    ImGui::BeginDisabled( !isChanged() );
    if ( ImGui::Button( "Apply", ImVec2( 60, 0 ) ) )
    {
      mManager.mImageProperties->setRotation( rotation );
      mManager.updateRotation();
      if ( eeprom.bits != mManager.mImageProperties->getEEPROM().bits )
      {
        mManager.mImageProperties->setEEPROM( eeprom.bits );
        mManager.mDoReset = true;
      }
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::SetItemDefaultFocus();
    if ( ImGui::Button( "Cancel", ImVec2( 60, 0 ) ) )
    {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}



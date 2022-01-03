#include "pch.hpp"
#include "Manager.hpp"
#include "InputFile.hpp"
#include "WinRenderer.hpp"
#include "WinImgui.hpp"
#include "WinAudioOut.hpp"
#include "ComLynxWire.hpp"
#include "Core.hpp"
#include "SymbolSource.hpp"
#include "imgui.h"
#include "Log.hpp"
#include "Ex.hpp"
#include "CPUState.hpp"
#include "IEncoder.hpp"
#include "ConfigProvider.hpp"
#include "SysConfig.hpp"
#include "ScriptDebuggerEscapes.hpp"
#include "UserInput.hpp"
#include "ImageROM.hpp"
#include "KeyNames.hpp"
#include "ImageProperties.hpp"
#include "LuaProxies.hpp"
#include "CPU.hpp"
#include "DebugRAM.hpp"
#include <imfilebrowser.h>

namespace
{


}

Manager::Manager() : mLua{}, mDoUpdate{ false }, mDebugger{}, mProcessThreads{}, mJoinThreads{},
  mRenderThread{}, mAudioThread{}, mRenderingTime{}, mOpenMenu{ false }, mFileBrowser{ std::make_unique<ImGui::FileBrowser>() },
  mScriptDebuggerEscapes{ std::make_shared<ScriptDebuggerEscapes>() }, mIntputSource{}, mKeyNames{ std::make_shared<KeyNames>() },
  mImageProperties{}
{
  mDebugger( RunMode::RUN );
  mRenderer = std::make_shared<WinRenderer>();
  mAudioOut = std::make_shared<WinAudioOut>( mDebugger.mRunMode );
  mComLynxWire = std::make_shared<ComLynxWire>();
  mIntputSource = std::make_shared<UserInput>( *gConfigProvider.sysConfig() );

  mRenderThread = std::thread{ [this]
  {
    while ( !mJoinThreads.load() )
    {
      if ( mProcessThreads.load() )
      {
        auto renderingTime = mRenderer->render( *this );
        std::scoped_lock<std::mutex> l{ mMutex };
        mRenderingTime = renderingTime;
      }
      else
      {
        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
      }
    }
  } };

  mAudioThread = std::thread{ [this]
  {
    try
    {
      while ( !mJoinThreads.load() )
      {
        if ( mProcessThreads.load() )
        {
          int64_t renderingTime;
          {
            std::scoped_lock<std::mutex> l{ mMutex };
            renderingTime = mRenderingTime;
          }
          mAudioOut->fillBuffer( mInstance, renderingTime );
          updateDebugWindows();
        }
        else
        {
          std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
      }
    }
  catch ( sol::error const& err )
  {
    L_ERROR << err.what();
    MessageBoxA( nullptr, err.what(), "Error", 0 );
    std::terminate();
  }
  catch ( std::exception const& ex )
  {
    L_ERROR << ex.what();
    MessageBoxA( nullptr, ex.what(), "Error", 0 );
    std::terminate();
  }
  } };
}

void Manager::update()
{
  mIntputSource->updateGamepad();

  if ( mDoUpdate )
    reset();
  mDoUpdate = false;
}

void Manager::doArg( std::wstring arg )
{
  mArg = std::filesystem::path{ std::move( arg ) };
  reset();
}

void Manager::initialize( HWND hWnd )
{
  mhWnd = hWnd;
  assert( mRenderer );
  mRenderer->initialize( hWnd, gConfigProvider.appDataFolder() );
}

int Manager::win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  RECT r;
  switch ( msg )
  {
  case WM_CLOSE:
    if ( GetWindowRect( hWnd, &r ) )
    {
      auto sysConfig = gConfigProvider.sysConfig();
      sysConfig->mainWindow.x = r.left;
      sysConfig->mainWindow.y = r.top;
      sysConfig->mainWindow.width = r.right - r.left;
      sysConfig->mainWindow.height = r.bottom - r.top;
    }
    DestroyWindow( hWnd );
    break;
  case WM_DESTROY:
    PostQuitMessage( 0 );
    break;
  case WM_DROPFILES:
    handleFileDrop( (HDROP)wParam );
    SetForegroundWindow( hWnd );
    reset();
    break;
  case WM_COPYDATA:
    if ( handleCopyData( std::bit_cast<COPYDATASTRUCT const*>( lParam ) ) )
    {
      reset();
      return true;
    }
    break;
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
    if ( wParam < 256 )
    {
      mIntputSource->keyDown( (int)wParam );
    }
    return mRenderer->win32_WndProcHandler( hWnd, msg, wParam, lParam );
  case WM_KEYUP:
  case WM_SYSKEYUP:
    if ( wParam < 256 )
    {
      mIntputSource->keyUp( (int)wParam );
    }
    return mRenderer->win32_WndProcHandler( hWnd, msg, wParam, lParam );
  case WM_KILLFOCUS:
    mIntputSource->lostFocus();
    return mRenderer->win32_WndProcHandler( hWnd, msg, wParam, lParam );
  case WM_DEVICECHANGE:
    if ( (UINT)wParam == DBT_DEVNODES_CHANGED )
      mIntputSource->recheckGamepad();
    return 0;
  default:
    assert( mRenderer );
    return mRenderer->win32_WndProcHandler( hWnd, msg, wParam, lParam );
  }

  return 0;
}

Manager::~Manager()
{
  mIntputSource->serialize( *gConfigProvider.sysConfig() );
  stopThreads();
}

void Manager::quit()
{
  PostMessage( mhWnd, WM_CLOSE, 0, 0 );
}

bool Manager::mainMenu( ImGuiIO& io )
{
  enum class FileBrowserAction
  {
    NONE,
    OPEN_CARTRIDGE,
    OPEN_BOOTROM
  };

  enum class ModalWindow
  {
    NONE,
    PROPERTIES
  };

  static ModalWindow modalWindow = ModalWindow::NONE;
  static FileBrowserAction fileBrowserAction = FileBrowserAction::NONE;
  static std::optional<KeyInput::Key> keyToConfigure;

  auto configureKeyItem = [&]( char const* name, KeyInput::Key k )
  {
    ImGui::Text( name );
    ImGui::SameLine( 60 );

    if ( ImGui::Button( mKeyNames->name( mIntputSource->getVirtualCode( k ) ), ImVec2( 100, 0 ) ) )
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
  bool debugMode = mDebugger.isDebugMode();

  if ( ImGui::IsKeyPressed( VK_F3 ) )
  {
    resetIssued = true;
  }

  if ( ImGui::IsKeyPressed( VK_F4 ) )
  {
    debugMode = !debugMode;
  }

  if ( ImGui::IsKeyPressed( VK_F5 ) )
  {
    pauseRunIssued = true;
  }
  else if ( ImGui::IsKeyPressed( VK_F6 ) )
  {
    stepInIssued = true;
  }
  else if ( ImGui::IsKeyPressed( VK_F7 ) )
  {
    stepOverIssued = true;
  }
  else if ( ImGui::IsKeyPressed( VK_F8 ) )
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
      if ( ImGui::MenuItem( "Open" ) )
      {
        mFileBrowser->SetTitle( "Open Cartridge image file" );
        mFileBrowser->SetTypeFilters( { ".lnx", ".lyx", ".o" } );
        mFileBrowser->Open();
        fileBrowserAction = FileBrowserAction::OPEN_CARTRIDGE;
      }
      if ( ImGui::MenuItem( "Exit", "Alt+F4" ) )
      {
        quit();
      }
      ImGui::EndMenu();
    }
    ImGui::BeginDisabled( !(bool)mImageProperties );
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
    if ( mRenderer->canRenderBoards() )
    {
      ImGui::BeginDisabled( !(bool)mInstance );
      if ( ImGui::BeginMenu( "Debug" ) )
      {
        openMenu = true;

        if ( ImGui::MenuItem( "Reset", "F3" ) )
        {
          resetIssued = true;
        }

        ImGui::MenuItem( "Debug Mode", "F4", &debugMode );

        if ( ImGui::MenuItem( mDebugger.isPaused() ? "Run" : "Break", "F5") )
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
          bool cpuWindow = mDebugger.isCPUVisualized();
          bool disasmWindow = mDebugger.isDisasmVisualized();
          bool historyWindow = mDebugger.isHistoryVisualized();
          if ( ImGui::MenuItem( "CPU Window", "Ctrl+C", &cpuWindow ) )
          {
            mDebugger.visualizeCPU( cpuWindow );
          }
          if ( ImGui::MenuItem( "Disassembly Window", "Ctrl+D", &disasmWindow ) )
          {
            mDebugger.visualizeDisasm( disasmWindow );
          }
          if ( ImGui::MenuItem( "History Window", "Ctrl+H", &historyWindow ) )
          {
            if ( historyWindow )
            {
              mInstance->debugCPU().enableHistory( mDebugger.historyVisualizer().columns, mDebugger.historyVisualizer().rows );
              mDebugger.visualizeHistory( true );
            }
            else
            {
              mInstance->debugCPU().disableHistory();
              mDebugger.visualizeHistory( false );
            }
          }
          ImGui::EndMenu();
        }
        if ( ImGui::BeginMenu( "Options" ) )
        {
          bool breakOnBrk = mInstance->debugCPU().isBreakOnBrk();
          bool debugModeOnBreak = mDebugger.debugModeOnBreak();
          bool normalModeOnRun = mDebugger.normalModeOnRun();

          if ( ImGui::MenuItem( "Break on BRK", nullptr, &breakOnBrk ) )
          {
            mInstance->debugCPU().breakOnBrk( breakOnBrk );
          }
          if ( ImGui::MenuItem( "Debug mode on break", nullptr, &debugModeOnBreak ) )
          {
            mDebugger.debugModeOnBreak( debugModeOnBreak );
          }
          if ( ImGui::MenuItem( "Normal mode on run", nullptr, &normalModeOnRun ) )
          {
            mDebugger.normalModeOnRun( normalModeOnRun );
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
            mDoUpdate = true;
          }
          if ( ImGui::MenuItem( "Clear boot ROM path" ) )
          {
            sysConfig->bootROM.path.clear();
            if ( sysConfig->bootROM.useExternal )
            {
              sysConfig->bootROM.useExternal = false;
              mDoUpdate = true;
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
    if ( ImGui::IsKeyPressed( 'P' ) )
    {
      modalWindow = ModalWindow::PROPERTIES;
    }
    if ( ImGui::IsKeyPressed( 'C' ) )
    {
      mDebugger.visualizeCPU( !mDebugger.isCPUVisualized() );
    }
    if ( ImGui::IsKeyPressed( 'D' ) )
    {
      mDebugger.visualizeDisasm( !mDebugger.isDisasmVisualized() );
    }
    if ( ImGui::IsKeyPressed( 'H' ) )
    {
      bool historyWindow = !mDebugger.isHistoryVisualized();
      if ( historyWindow )
      {
        mInstance->debugCPU().enableHistory( mDebugger.historyVisualizer().columns, mDebugger.historyVisualizer().rows );
        mDebugger.visualizeHistory( true );
      }
      else
      {
        mInstance->debugCPU().disableHistory();
        mDebugger.visualizeHistory( false );
      }
    }
  }

  mDebugger.debugMode( debugMode );

  if ( resetIssued )
  {
    reset();
    mDebugger( RunMode::PAUSE );
  }
  if ( stepOutIssued )
  {
    mDebugger( RunMode::STEP_OUT );
  }
  if ( stepOverIssued )
  {
    mDebugger( RunMode::STEP_OVER );
  }
  if ( stepInIssued )
  {
    mDebugger( RunMode::STEP_IN );
  }
  else if ( pauseRunIssued )
  {
    mDebugger.togglePause();
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


  if ( auto openPath = gConfigProvider.sysConfig()->lastOpenDirectory; !openPath.empty() )
  {
    mFileBrowser->SetPwd( gConfigProvider.sysConfig()->lastOpenDirectory );
  }
  mFileBrowser->Display();
  if ( mFileBrowser->HasSelected() )
  {
    using enum FileBrowserAction;
    switch ( fileBrowserAction )
    {
    case OPEN_CARTRIDGE:
      mArg = mFileBrowser->GetSelected();
      if ( auto parent = mArg.parent_path(); !parent.empty() )
      {
        gConfigProvider.sysConfig()->lastOpenDirectory = mArg.parent_path();
      }
      mDoUpdate = true;
      break;
    case OPEN_BOOTROM:
      sysConfig->bootROM.path = mFileBrowser->GetSelected();
      break;
    }
    mFileBrowser->ClearSelected();
    fileBrowserAction = NONE;
  }

  return openMenu;
}

void Manager::configureKeyWindow( std::optional<KeyInput::Key>& keyToConfigure )
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
        code = mIntputSource->getVirtualCode( *keyToConfigure );
      }
      if ( auto c = mIntputSource->firstKeyPressed() )
      {
        code = c;
      }
      ImGui::Text( mKeyNames->name( code ) );
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      if ( ImGui::Button( "OK", ImVec2( 60, 0 ) ) )
      {
        mIntputSource->updateMapping( *keyToConfigure, code );
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

void Manager::imagePropertiesWindow( bool init )
{
  if ( ImGui::BeginPopupModal( "Image properties", NULL, ImGuiWindowFlags_AlwaysAutoResize ) )
  {
    static int rotation;
    static ImageProperties::EEPROM eeprom;
    if ( init )
    {
      rotation = (int)mImageProperties->getRotation();
      eeprom = mImageProperties->getEEPROM();
    }

    auto isChanged = [&]
    {
      if ( rotation != (int)mImageProperties->getRotation() )
        return true;
      if ( eeprom.bits != mImageProperties->getEEPROM().bits )
        return true;

      return false;
    };

    auto cartName = mImageProperties->getCartridgeName();
    auto manufName = mImageProperties->getMamufacturerName();
    auto const& bankProps = mImageProperties->getBankProps();
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
    ImGui::TextUnformatted( mImageProperties->getAUDInUsed() ? "Yes" : "No" );
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
      mImageProperties->setRotation( rotation );
      updateRotation();
      if ( eeprom.bits != mImageProperties->getEEPROM().bits )
      {
        mImageProperties->setEEPROM( eeprom.bits );
        mDoUpdate = true;
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

void Manager::renderBoard( DebugWindow& win )
{
  auto tex = mRenderer->renderBoard( win.id, win.columns, win.rows, std::span<uint8_t const>{ win.data.data(), win.data.size() } );
  ImGui::Image( tex, ImVec2{ 8.0f * win.columns , 16.0f * win.rows } );
}

void Manager::drawDebugWindows( ImGuiIO& io )
{
  std::unique_lock<std::mutex> l{ mDebugger.mutex };

  bool cpuWindow = mDebugger.isCPUVisualized();
  bool disasmWindow = mDebugger.isDisasmVisualized();
  bool historyWindow = mDebugger.isHistoryVisualized();
  bool debugMode = mDebugger.isDebugMode();

  if ( debugMode )
  {
    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2{ 2.0f, 2.0f } );

    if ( cpuWindow )
    {
      ImGui::Begin( "CPU", &cpuWindow, ImGuiWindowFlags_AlwaysAutoResize );
      renderBoard( mDebugger.cpuVisualizer() );
      ImGui::End();
    }

    if ( disasmWindow )
    {
      ImGui::Begin( "Disassembly", &disasmWindow, ImGuiWindowFlags_AlwaysAutoResize );
      renderBoard( mDebugger.disasmVisualizer() );
      ImGui::End();
    }

    if ( historyWindow )
    {
      ImGui::Begin( "History", &historyWindow, ImGuiWindowFlags_AlwaysAutoResize );
      renderBoard( mDebugger.historyVisualizer() );
      ImGui::End();
    }

    {
      static const float xpad = 4.0f;
      static const float ypad = 4.0f + 19.0f;
      ImGui::PushStyleVar( ImGuiStyleVar_WindowMinSize, ImVec2{ 160.0f + xpad, 102.0f + ypad } );

      ImGui::Begin( "Rendering", &debugMode, 0 );
      auto size = ImGui::GetWindowSize();
      size.x -= xpad;
      size.y -= ypad;
      if ( auto tex = mRenderer->mainRenderingTexture( (int)size.x, (int)size.y ) )
      {
        ImGui::Image( tex, size );
      }
      ImGui::End();
      mDebugger.debugMode( debugMode );
      ImGui::PopStyleVar();
    }
    ImGui::PopStyleVar();


    if ( ImGui::BeginPopupContextVoid() )
    {
      bool breakOnBrk = mInstance->debugCPU().isBreakOnBrk();

      if ( ImGui::Checkbox( "CPU Window", &cpuWindow ) )
      {
        mDebugger.visualizeCPU( cpuWindow );
      }
      if ( ImGui::Checkbox( "Disassembly Window", &disasmWindow ) )
      {
        mDebugger.visualizeDisasm( disasmWindow );
      }
      if ( ImGui::Checkbox( "History Window", &historyWindow ) )
      {
        mDebugger.visualizeHistory( historyWindow );
        if ( historyWindow )
        {
          mInstance->debugCPU().enableHistory( mDebugger.historyVisualizer().columns, mDebugger.historyVisualizer().rows );
        }
        else
        {
          mInstance->debugCPU().disableHistory();
        }
      }
      if ( ImGui::Checkbox( "Break on BRK", &breakOnBrk ) )
      {
        mInstance->debugCPU().breakOnBrk( breakOnBrk );
      }
      ImGui::EndPopup();
    }
  }
  else
  {
    mRenderer->mainRenderingTexture( 0, 0 );
  }
}

void Manager::updateDebugWindows()
{
  std::unique_lock<std::mutex> l{ mDebugger.mutex };

  if ( !mInstance )
    return;

  auto& cpu = mInstance->debugCPU();

  if ( mDebugger.isCPUVisualized() )
  {
    auto & cpuVis = mDebugger.cpuVisualizer();
    cpu.printStatus( std::span<uint8_t, 3 * 14>( cpuVis.data.data(), cpuVis.data.size() ) );
  }

  if ( mDebugger.isDisasmVisualized() )
  {
    auto & disVis = mDebugger.disasmVisualizer();
    cpu.disassemblyFromPC( mInstance->debugRAM(), (char*)disVis.data.data(), disVis.columns, disVis.rows );
  }

  if ( mDebugger.isHistoryVisualized() )
  {
    auto & hisVis = mDebugger.historyVisualizer();
    cpu.copyHistory( std::span<char>( (char*)hisVis.data.data(), hisVis.data.size() ) );
  }
}

void Manager::drawGui( int left, int top, int right, int bottom )
{
  ImGuiIO & io = ImGui::GetIO();

  bool hovered = io.MousePos.x > left && io.MousePos.y > top && io.MousePos.x < right && io.MousePos.y < bottom;

  if ( hovered || mOpenMenu )
  {
    mOpenMenu = mainMenu( io );
  }

  drawDebugWindows( io );
}

void Manager::processLua( std::filesystem::path const& path )
{
  auto luaPath = path;
  auto cfgPath = path;

  luaPath.replace_extension( path.extension().string() + ".lua" );
  cfgPath.replace_extension( path.extension().string() + ".cfg" );

  if ( !std::filesystem::exists( luaPath ) && !std::filesystem::exists( cfgPath ) )
    return;

  mLua = sol::state{};
  mLua.open_libraries( sol::lib::base, sol::lib::io );

  if ( std::filesystem::exists( cfgPath ) )
  {
    mLua.safe_script_file( cfgPath.string(), sol::script_pass_on_error );
    //ignoring errors
  }

  if ( !std::filesystem::exists( luaPath ) )
    return;

  mLua.new_usertype<TrapProxy>( "TRAP", sol::meta_function::new_index, &TrapProxy::set );
  mLua.new_usertype<RamProxy>( "RAM", sol::meta_function::index, &RamProxy::get, sol::meta_function::new_index, &RamProxy::set );
  mLua.new_usertype<RomProxy>( "ROM", sol::meta_function::index, &RomProxy::get, sol::meta_function::new_index, &RomProxy::set );
  mLua.new_usertype<MikeyProxy>( "MIKEY", sol::meta_function::index, &MikeyProxy::get, sol::meta_function::new_index, &MikeyProxy::set );
  mLua.new_usertype<SuzyProxy>( "SUZY", sol::meta_function::index, &SuzyProxy::get, sol::meta_function::new_index, &SuzyProxy::set );
  mLua.new_usertype<CPUProxy>( "CPU", sol::meta_function::index, &CPUProxy::get, sol::meta_function::new_index, &CPUProxy::set );

  mLua["ram"] = std::make_unique<RamProxy>( *this );
  mLua["rom"] = std::make_unique<RomProxy>( *this );
  mLua["mikey"] = std::make_unique<MikeyProxy>( *this );
  mLua["suzy"] = std::make_unique<SuzyProxy>( *this );
  mLua["cpu"] = std::make_unique<CPUProxy>( *this );

  mLua["Encoder"] = [this]( sol::table const& tab )
  {
    std::filesystem::path path;
    int vbitrate{}, abitrate{}, vscale{};
    if ( sol::optional<std::string> opt = tab["path"] )
      path = *opt;
    else throw Ex{} << "path = \"path/to/file.mp4\" required";

    if ( sol::optional<int> opt = tab["video_bitrate"] )
      vbitrate = *opt;
    else throw Ex{} << "video_bitrate required";

    if ( sol::optional<int> opt = tab["audio_bitrate"] )
      abitrate = *opt;
    else throw Ex{} << "audio_bitrate required";

    if ( sol::optional<int> opt = tab["video_scale"] )
      vscale = *opt;
    else throw Ex{} << "video_scale required";

    if ( vscale % 2 == 1 )
      throw Ex{} << "video_scale must be even number";

    static PCREATE_ENCODER s_createEncoder = nullptr;
    static PDISPOSE_ENCODER s_disposeEncoder = nullptr;

    mEncoderMod = ::LoadLibrary( L"Encoder.dll" );
    if ( mEncoderMod == nullptr )
      throw Ex{} << "Encoder.dll not found";

    s_createEncoder = (PCREATE_ENCODER)GetProcAddress( mEncoderMod, "createEncoder" );
    s_disposeEncoder = (PDISPOSE_ENCODER)GetProcAddress( mEncoderMod, "disposeEncoder" );

    mEncoder = std::shared_ptr<IEncoder>( s_createEncoder( path.string().c_str(), vbitrate, abitrate, 160 * vscale, 102 * vscale ), s_disposeEncoder );
    mRenderer->setEncoder( mEncoder );
    mAudioOut->setEncoder( mEncoder );
  };

  mLua["WavOut"] = [this]( sol::table const& tab )
  {
    std::filesystem::path path;
    if ( sol::optional<std::string> opt = tab["path"] )
      path = *opt;
    else throw Ex{} << "path = \"path/to/file.wav\" required";

    mAudioOut->setWavOut( std::move( path ) );
  };

  mLua["traceCurrent"] = [this]()
  {
    if ( mInstance )
    {
      mInstance->debugCPU().toggleTrace( true );
    }
  };

  mLua["traceOn"] = [this]()
  {
    mInstance->debugCPU().enableTrace();
  };
  mLua["traceOf"] = [this]()
  {
    mInstance->debugCPU().disableTrace();
  };
  mLua["break"] = [this]()
  {

  };

  mLua.script_file( luaPath.string() );

  if ( sol::optional<std::string> opt = mLua["log"] )
  {
    mLogPath = *opt;
  }
  if ( sol::optional<std::string> opt = mLua["lab"] )
  {
    mSymbols = std::make_unique<SymbolSource>( *opt );
  }

}

std::optional<InputFile> Manager::computeInputFile()
{
  std::optional<InputFile> input;

  std::filesystem::path path = std::filesystem::absolute( mArg );

  InputFile file{ path, mImageProperties };
  if ( !file.valid() )
    return {};

  processLua( path );

  return file;
}

std::shared_ptr<ImageROM const> Manager::getOptionalBootROM()
{
  auto sysConfig = gConfigProvider.sysConfig();
  if ( sysConfig->bootROM.useExternal && !sysConfig->bootROM.path.empty() )
  {
    return ImageROM::create( sysConfig->bootROM.path );
  }

  return {};
}

void Manager::reset()
{
  std::unique_lock<std::mutex> l{ mDebugger.mutex };
  mProcessThreads.store( false );
  //TODO wait for threads to stop.
  mInstance.reset();

  if ( auto input = computeInputFile() )
  {
    mInstance = std::make_shared<Core>( *mImageProperties, mComLynxWire, mRenderer->getVideoSink(), mIntputSource,
      *input, getOptionalBootROM(), mScriptDebuggerEscapes );

    updateRotation();

    if ( !mLogPath.empty() )
      mInstance->setLog( mLogPath );
  }
  else
  {
    mImageProperties.reset();
  }

  if ( mInstance )
  {
    if ( mDebugger.isHistoryVisualized() )
    {
      mInstance->debugCPU().enableHistory( mDebugger.historyVisualizer().columns, mDebugger.historyVisualizer().rows );
    }
    else
    {
      mInstance->debugCPU().disableHistory();
    }
  }

  mProcessThreads.store( true );
  mDebugger( RunMode::RUN );
}

void Manager::updateRotation()
{
  mIntputSource->setRotation( mImageProperties->getRotation() );
  mRenderer->setRotation( mImageProperties->getRotation() );
}

void Manager::stopThreads()
{
  mJoinThreads.store( true );
  if ( mAudioThread.joinable() )
    mAudioThread.join();
  mAudioThread = {};
  if ( mRenderThread.joinable() )
    mRenderThread.join();
  mRenderThread = {};
}

void Manager::handleFileDrop( HDROP hDrop )
{
#ifdef _WIN64
  auto h = GlobalAlloc( GMEM_MOVEABLE, 0 );
  uintptr_t hptr = reinterpret_cast<uintptr_t>( h );
  GlobalFree( h );
  uintptr_t hdropptr = reinterpret_cast<uintptr_t>( hDrop );
  hDrop = reinterpret_cast<HDROP>( hptr & 0xffffffff00000000 | hdropptr & 0xffffffff );
#endif

  uint32_t cnt = DragQueryFile( hDrop, ~0, nullptr, 0 );

  if ( cnt > 0 )
  {
    uint32_t size = DragQueryFile( hDrop, 0, nullptr, 0 );
    std::wstring arg( size + 1, L'\0' );
    DragQueryFile( hDrop, 0, arg.data(), size + 1 );
    mArg = arg;
  }

  DragFinish( hDrop );
  mDoUpdate = true;
}

bool Manager::handleCopyData( COPYDATASTRUCT const* copy )
{
  if ( copy )
  {
    std::span<wchar_t const> span{ (wchar_t const*)copy->lpData, copy->cbData / sizeof( wchar_t ) };
    
    wchar_t const* ptr = span.data();
    if ( size_t size = std::wcslen( ptr ) )
    {
      mArg = { ptr, size };
      return true;
    }
  }

  return false;
}

Manager::Debugger::Debugger() : mutex{}, mDebugMode{}, mVisualizeCPU{}, mVisualizeDisasm{}, mVisualizeHistory{}, mDebugModeOnBreak{}, mNormalModeOnRun{}, mCpuVisualizer{ 0, 14, 3 }, mDisasmVisualizer{ 1, 32, 16 }, mHistoryVisualizer{ 2, 64, 16 }, mRunMode{ RunMode::RUN }
{
  auto sysConfig = gConfigProvider.sysConfig();

  mDebugMode = sysConfig->debugMode;
  mVisualizeCPU = sysConfig->visualizeCPU;
  mVisualizeDisasm = sysConfig->visualizeDisasm;
  mVisualizeHistory = sysConfig->visualizeHistory;
  mDebugModeOnBreak = sysConfig->debugModeOnBreak;
  mNormalModeOnRun = sysConfig->normalModeOnRun;
}

Manager::Debugger::~Debugger()
{
  auto sysConfig = gConfigProvider.sysConfig();

  sysConfig->debugMode = mDebugMode;
  sysConfig->visualizeCPU = mVisualizeCPU;
  sysConfig->visualizeDisasm = mVisualizeDisasm;
  sysConfig->visualizeHistory = mVisualizeHistory;
  sysConfig->debugModeOnBreak = mDebugModeOnBreak;
  sysConfig->normalModeOnRun = mNormalModeOnRun;
}

void Manager::Debugger::operator()( RunMode mode )
{
  if ( mode == RunMode::RUN && mNormalModeOnRun )
  {
    mDebugMode = false;
  }
  if ( mode == RunMode::PAUSE && mDebugModeOnBreak )
  {
    mDebugMode = true;
  }

  mRunMode.store( mode );
}

bool Manager::Debugger::isPaused() const
{
  return mRunMode.load() == RunMode::PAUSE;
}

bool Manager::Debugger::isDebugMode() const
{
  return mDebugMode;
}

bool Manager::Debugger::isCPUVisualized() const
{
  return mVisualizeCPU;
}

bool Manager::Debugger::isDisasmVisualized() const
{
  return mVisualizeDisasm;
}

bool Manager::Debugger::isHistoryVisualized() const
{
  return mVisualizeHistory;
}

void Manager::Debugger::visualizeCPU( bool value )
{
  mVisualizeCPU = value;
}

void Manager::Debugger::visualizeDisasm( bool value )
{
  mVisualizeDisasm = value;
}

void Manager::Debugger::visualizeHistory( bool value )
{
  mVisualizeHistory = value;
}

void Manager::Debugger::debugMode( bool value )
{
  mDebugMode = value;
}

bool Manager::Debugger::debugModeOnBreak() const
{
  return mDebugModeOnBreak;
}

void Manager::Debugger::debugModeOnBreak( bool value )
{
  mDebugModeOnBreak = value;
}

bool Manager::Debugger::normalModeOnRun() const
{
  return mNormalModeOnRun;
}

void Manager::Debugger::normalModeOnRun( bool value )
{
  mNormalModeOnRun = value;
}

void Manager::Debugger::togglePause()
{
  if ( mRunMode.load() == RunMode::PAUSE )
  {
    ( *this )( RunMode::RUN );
  }
  else
  {
    ( *this )( RunMode::PAUSE );
  }
}

Manager::DebugWindow& Manager::Debugger::cpuVisualizer()
{
  return mCpuVisualizer;
}

Manager::DebugWindow& Manager::Debugger::disasmVisualizer()
{
  return mDisasmVisualizer;
}

Manager::DebugWindow& Manager::Debugger::historyVisualizer()
{
  return mHistoryVisualizer;
}

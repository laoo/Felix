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
#include "WinConfig.hpp"
#include "SysConfig.hpp"
#include "ScriptDebuggerEscapes.hpp"
#include "UserInput.hpp"
#include "ImageROM.hpp"
#include "KeyNames.hpp"
#include "ImageProperties.hpp"
#include "LuaProxies.hpp"
#include <imfilebrowser.h>

namespace
{


}

Manager::Manager() : mLua{}, mDoUpdate{ false }, mProcessThreads{}, mJoinThreads{}, mPaused{},
  mRenderThread{}, mAudioThread{}, mLogStartCycle{}, mRenderingTime{}, mOpenMenu{ false }, mFileBrowser{ std::make_unique<ImGui::FileBrowser>() },
  mScriptDebuggerEscapes{ std::make_shared<ScriptDebuggerEscapes>() }, mIntputSource{}, mKeyNames{ std::make_shared<KeyNames>() },
  mImageProperties{}
{
  mRenderer = std::make_shared<WinRenderer>();
  mAudioOut = std::make_shared<WinAudioOut>();
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
          if ( !mPaused.load() )
          {
            int64_t renderingTime;
            {
              std::scoped_lock<std::mutex> l{ mMutex };
              renderingTime = mRenderingTime;
            }
            mAudioOut->fillBuffer( mInstance, renderingTime );
          }
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
      auto winConfig = gConfigProvider.winConfig();
      winConfig->mainWindow.x = r.left;
      winConfig->mainWindow.y = r.top;
      winConfig->mainWindow.width = r.right - r.left;
      winConfig->mainWindow.height = r.bottom - r.top;
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
  gConfigProvider.serialize();
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
    if ( ImGui::IsKeyDown( 'P' ) )
    {
      modalWindow = ModalWindow::PROPERTIES;
    }
  }

  switch ( modalWindow )
  {
  case ModalWindow::PROPERTIES:
    ImGui::OpenPopup( "Image properties" );
    break;
  default:
    break;
  }

  imagePropertiesWindow( modalWindow != ModalWindow::NONE );

  modalWindow = ModalWindow::NONE;


  mFileBrowser->Display();
  if ( mFileBrowser->HasSelected() )
  {
    using enum FileBrowserAction;
    switch ( fileBrowserAction )
    {
    case OPEN_CARTRIDGE:
      mArg = mFileBrowser->GetSelected();
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
    static bool changed;
    static int rotation;
    if ( init )
    {
      changed = false;
      rotation = (int)mImageProperties->getRotation();
      if ( rotation < 0 || rotation > 2 )
        rotation = 0;
    }

    auto cartName = mImageProperties->getCartridgeName();
    auto manufName = mImageProperties->getMamufacturerName();
    auto eeprom = mImageProperties->getEEPROM();
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
    if ( ImGui::Combo( "", &rotation, "Normal\0Left\0Right\0\0" ) )
    {
      changed = rotation != (int)mImageProperties->getRotation();
    }
    ImGui::TextUnformatted( "EEPROM:" );
    ImGui::SameLine();
    ImGui::TextUnformatted( eeprom.name().data(), eeprom.name().data() + eeprom.name().size() );
    if ( eeprom.type() != 0 )
    {
      ImGui::TextUnformatted( "EEPROM bitness:" );
      ImGui::SameLine();
      ImGui::TextUnformatted( eeprom.is16Bit() ? "16-bit" : "8-bit" );
    }
    ImGui::TextUnformatted( "SD Card support:" );
    ImGui::SameLine();
    ImGui::TextUnformatted( eeprom.sd() ? "Yes" : "No" );
    ImGui::BeginDisabled( !changed );
    if ( ImGui::Button( "OK", ImVec2( 60, 0 ) ) )
    {
      mImageProperties->setRotation( rotation );
      updateRotation();
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

void Manager::drawGui( int left, int top, int right, int bottom )
{
  ImGuiIO & io = ImGui::GetIO();

  bool hovered = io.MousePos.x > left && io.MousePos.y > top && io.MousePos.x < right && io.MousePos.y < bottom;

  if ( hovered || mOpenMenu )
  {
    mOpenMenu = mainMenu( io );
  }
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

  mLua["ram"] = std::make_unique<RamProxy>( *this );
  mLua["rom"] = std::make_unique<RomProxy>( *this );
  mLua["mikey"] = std::make_unique<MikeyProxy>( *this );
  mLua["suzy"] = std::make_unique<SuzyProxy>( *this );

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

  mLua["Log"] = [this]( sol::table const& tab )
  {
    if ( sol::optional<std::string> opt = tab["path"] )
    {
      mLogPath = *opt;
    }
    else
    {
      throw Ex{} << "path = \"path/to/log\" required";
    }

    if ( sol::optional<uint64_t> opt = tab["start_tick"] )
    {
      mLogStartCycle = *opt;
    }
  };

  mLua.script_file( luaPath.string() );

  if ( sol::optional<std::string> opt = mLua["log"] )
  {
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

  assert( mImageProperties );
  InputFile file{ path, *mImageProperties };
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
  mProcessThreads.store( false );
  //TODO wait for threads to stop.
  mInstance.reset();
  mImageProperties = std::make_shared<ImageProperties>( std::filesystem::path{ mArg } );

  if ( auto input = computeInputFile() )
  {
    mInstance = std::make_shared<Core>( *mImageProperties, mComLynxWire, mRenderer->getVideoSink(), mIntputSource,
      *input, getOptionalBootROM(), mScriptDebuggerEscapes );

    updateRotation();

    if ( !mLogPath.empty() )
      mInstance->setLog( mLogPath, mLogStartCycle );
  }
  else
  {
    mImageProperties.reset();
  }

  mProcessThreads.store( true );
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


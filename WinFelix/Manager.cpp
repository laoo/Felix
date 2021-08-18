#include "pch.hpp"
#include "Manager.hpp"
#include "InputFile.hpp"
#include "WinRenderer.hpp"
#include "WinAudioOut.hpp"
#include "ComLynxWire.hpp"
#include "Core.hpp"
#include "Monitor.hpp"
#include "SymbolSource.hpp"
#include "imgui.h"
#include "Log.hpp"
#include "Ex.hpp"
#include "CPUState.hpp"
#include "VideoEncoder.hpp"

Manager::Manager() : mEmulationRunning{ true }, mHorizontalView{ true }, mDoUpdate{ false }, mIntputSources{}, mProcessThreads{ true },
  mInstancesCount{ 1 }, mRenderThread{}, mAudioThread{}, mAppDataFolder{ getAppDataFolder() }, mPaused{}, mLogStartCycle{}, mLua{}
{
  mIntputSources[0] = std::make_shared<InputSource>();
  mIntputSources[1] = std::make_shared<InputSource>();

  mRenderer = std::make_shared<WinRenderer>();
  mAudioOut = std::make_shared<WinAudioOut>();
  mComLynxWire = std::make_shared<ComLynxWire>();

  std::filesystem::create_directories( mAppDataFolder );
  mWinConfig = WinConfig::load( mAppDataFolder );

  mLua.open_libraries( sol::lib::base, sol::lib::io );
}

void Manager::update()
{
  processKeys();

  if ( mDoUpdate )
    reset();
  mDoUpdate = false;
}

void Manager::doArgs( std::vector<std::wstring> args )
{
  mArgs = std::move( args );
  reset();
}

WinConfig const& Manager::getWinConfig()
{
  return mWinConfig;
}

void Manager::initialize( HWND hWnd )
{
  assert( mRenderer );
  mRenderer->initialize( hWnd, mAppDataFolder );
}

int Manager::win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  RECT r;
  switch ( msg )
  {
  case WM_CLOSE:
    GetWindowRect( hWnd, &r );
    mWinConfig.mainWindow.x = r.left;
    mWinConfig.mainWindow.y = r.top;
    mWinConfig.mainWindow.width = r.right - r.left;
    mWinConfig.mainWindow.height = r.bottom - r.top;
    mWinConfig.serialize( mAppDataFolder );
    DestroyWindow( hWnd );
    break;
  case WM_DESTROY:
    PostQuitMessage( 0 );
    break;
  case WM_DROPFILES:
    handleFileDrop( (HDROP)wParam );
    reset();
    break;
  case WM_COPYDATA:
    if ( handleCopyData( std::bit_cast<COPYDATASTRUCT const*>( lParam ) ) )
    {
      reset();
      return true;
    }
    break;
  default:
    assert( mRenderer );
    return mRenderer->win32_WndProcHandler( hWnd, msg, wParam, lParam );
  }

  return 0;
}

Manager::~Manager()
{
  stopThreads();
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
      if ( ImGui::BeginMenu( "Options" ) )
      {
        bool doubleInstance = mInstancesCount > 1;
        if ( ImGui::Checkbox( "Double Instance", &doubleInstance ) )
        {
          mInstancesCount = doubleInstance ? 2 : 1;
          mDoUpdate = true;
        }
        
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
      ImGui::PopStyleVar();
    }
    ImGui::PopStyleVar();
  }

  if ( mMonitor && mInstances[0] )
  {
    if ( ImGui::Begin( "Monitor" ) )
    {
      ImGui::Text( "Tick: %u", mInstances[0]->tick() );
      for ( auto sv : mMonitor->sample( *mInstances[0] ) )
        ImGui::Text( sv.data() );
      if ( mMonit )
      {
        std::scoped_lock<std::mutex> l{ mMutex };
        mMonit.call( []( std::string const& txt )
        {
          ImGui::Text( txt.c_str() );
        } );
      }
    }
    ImGui::End();
  }

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

  if ( keys[VK_F9] & 0x80 )
  {
    mPaused = false;
    L_INFO << "UnPause";
  }
  if ( keys[VK_F10] & 0x80 )
  {
    mPaused = true;
    L_INFO << "Pause";
  }

}

std::filesystem::path Manager::getAppDataFolder()
{
  wchar_t* path_tmp;
  auto ret = SHGetKnownFolderPath( FOLDERID_LocalAppData, 0, nullptr, &path_tmp );

  if ( ret == S_OK )
  {
    std::filesystem::path result = path_tmp;
    CoTaskMemFree( path_tmp );
    return result / APP_NAME;
  }
  else
  {
    CoTaskMemFree( path_tmp );
    return {};
  }
}

std::shared_ptr<IInputSource> Manager::getInputSource( int instance )
{
  if ( instance >= 0 && instance < mIntputSources.size() )
    return mIntputSources[instance];
  else
    return {};
}

void Manager::Escape::call( uint8_t data, IAccessor & accessor )
{
  if ( data < manager.mEscapes.size() && manager.mEscapes[data].valid() )
  {
    std::scoped_lock<std::mutex> l{ manager.mMutex };
    manager.mEscapes[data].call( &accessor );
  }
}


void Manager::processLua( std::filesystem::path const& path, std::vector<InputFile>& inputs )
{
  auto decHex = [this]( sol::table const& tab, bool hex )
  {
    Monitor::Entry e{ hex };

    if ( sol::optional<std::string> opt = tab["label"] )
      e.name = *opt;
    else throw Ex{} << "Monitor entry required label";

    if ( sol::optional<int> opt = tab["size"] )
      e.size = *opt;

    return e;
  };

  mLua["Dec"] = [&]( sol::table const& tab ) { return decHex( tab, false ); };
  mLua["Hex"] = [&]( sol::table const& tab ) { return decHex( tab, true ); };

  mLua["Monitor"] = [this]( sol::table const& tab )
  {
    std::vector<Monitor::Entry> entries;

    for ( auto kv : tab )
    {
      sol::object const& value = kv.second;
      sol::type t = value.get_type();

      switch ( t )
      {
      case sol::type::userdata:
        if ( value.is<Monitor::Entry>() )
        {
          entries.push_back( value.as<Monitor::Entry>() );
        }
        else throw Ex{} << "Unknown type in Monitor";
        break;
      default:
        throw Ex{} << "Unsupported argument to Monitor";
      }
    }

    mMonitor = std::make_unique<Monitor>( std::move( entries ) );
  };

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

    mEncoder.reset( new VideoEncoder( path, vbitrate, abitrate, 160 * vscale, 102 * vscale ) );
    mRenderer->setEncoder( mEncoder );
    mAudioOut->setEncoder( mEncoder );
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

  mLua["tick"] = [this]( IEscape::IAccessor * acc ) { return acc->state().tick; };
  mLua["getA"] = [this]( IEscape::IAccessor * acc ) { return std::string( (char*)&acc->state().a, 1 ); };
  mLua["getX"] = [this]( IEscape::IAccessor * acc ) { return acc->state().x; };
  mLua["getY"] = [this]( IEscape::IAccessor * acc ) { return acc->state().y; };
  mLua["setA"] = [this]( IEscape::IAccessor * acc, int a ) { acc->state().a = (uint8_t)a; };
  mLua["setX"] = [this]( IEscape::IAccessor * acc, int x ) { acc->state().x = (uint8_t)x; };
  mLua["setY"] = [this]( IEscape::IAccessor * acc, int y ) { acc->state().y = (uint8_t)y; };
  mLua["peek"] = [this]( IEscape::IAccessor * acc, uint16_t addr ) { return acc->readRAM( addr ); };
  mLua["poke"] = [this]( IEscape::IAccessor * acc, uint16_t addr, uint8_t value ) { return acc->writeRAM( addr, value ); };

  mLua.script_file( path.string() );

  if ( sol::optional<std::string> opt = mLua["lnx"] )
  {
    InputFile file{ *opt };
    if ( file.valid() )
    {
      inputs.push_back( file );
    }
  }
  else
  {
    throw Ex{} << "Set lnx file using 'lnx = path'";
  }

  if ( sol::optional<std::string> opt = mLua["log"] )
  {
  }
  if ( sol::optional<std::string> opt = mLua["lab"] )
  {
    mSymbols = std::make_unique<SymbolSource>( *opt );
  }

  if ( mSymbols )
  {
    if ( mMonitor )
      mMonitor->populateSymbols( *mSymbols );
  }
  else
  {
    mMonitor.reset();
  }

  if ( auto esc = mLua["escapes"]; esc.valid() && esc.get_type() == sol::type::table )
  {
    auto escTab = mLua.get<sol::table>( "escapes" );
    for ( auto kv : escTab )
    {
      sol::object const& value = kv.second;
      sol::type t = value.get_type();

      if ( t == sol::type::function )
      {
        size_t id = kv.first.as<size_t>();
        while ( mEscapes.size() <= id )
          mEscapes.push_back( sol::nil );

        mEscapes[id] = (sol::function)value;
      }
    }
  }

  if ( auto esc = mLua["monit"]; esc.valid() && esc.get_type() == sol::type::function )
  {
    mMonit = mLua.get<sol::function>( "monit" );
  }
}

void Manager::reset()
{
  stopThreads();
  mProcessThreads.store( true );
  mInstances.clear();

  std::vector<InputFile> inputs;

  for ( auto const& arg : mArgs )
  {
    std::filesystem::path path{ arg };
    path = std::filesystem::absolute( path );
    if ( path.has_extension() && path.extension() == ".lua" )
    {
      processLua( path, inputs );
      continue;
    }

    InputFile file{ path };
    if ( file.valid() )
    {
      inputs.push_back( file );
    }
  }

  if ( !inputs.empty() )
  {

    mRenderer->setInstances( mInstancesCount );

    for ( size_t i = 0; i < mInstancesCount; ++i )
    {
      mInstances.push_back( std::make_shared<Core>( mComLynxWire, mRenderer->getVideoSink( (int)i ), getInputSource( (int)i ), std::span<InputFile>{ inputs.data(), inputs.size() } ) );
      if ( !mLogPath.empty() )
        mInstances.back()->setLog( mLogPath, mLogStartCycle );
    }

    mInstances[0]->setEscape( 0, std::make_shared<Escape>( *this ) );

    mRenderThread = std::thread{ [this]
    {
      while ( mProcessThreads.load() )
      {
        mRenderer->render( *this );
      }
    } };

    mAudioThread = std::thread{ [this]
    {
      while ( mProcessThreads.load() )
      {
        if ( !mPaused.load() )
          mAudioOut->fillBuffer( std::span<std::shared_ptr<Core> const>{ mInstances.data(), mInstances.size() } );
      }
    } };
  }
}

void Manager::stopThreads()
{
  mProcessThreads.store( 0 );
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
  mArgs.resize( cnt );

  for ( uint32_t i = 0; i < cnt; ++i )
  {
    uint32_t size = DragQueryFile( hDrop, i, nullptr, 0 );
    mArgs[i].resize( size + 1 );
    DragQueryFile( hDrop, i, mArgs[i].data(), size + 1 );
  }

  DragFinish( hDrop );
  mDoUpdate = true;
}

bool Manager::handleCopyData( COPYDATASTRUCT const* copy )
{
  if ( copy )
  {
    std::span<wchar_t const> span{ (wchar_t const*)copy->lpData, copy->cbData / sizeof( wchar_t ) };

    mArgs.clear();
    for ( wchar_t const* ptr = span.data(); ptr < span.data() + span.size(); )
    {
      if ( size_t size = std::wcslen( ptr ) )
      {
        mArgs.push_back( { ptr, size } );
        ptr += size;
      }
      else
      {
        break;
      }
    }

    return true;
  }
  return false;
}


Manager::InputSource::InputSource() : KeyInput{}
{
}

KeyInput Manager::InputSource::getInput() const
{
  return *this;
}

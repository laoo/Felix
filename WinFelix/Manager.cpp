#include "pch.hpp"
#include "Manager.hpp"
#include "InputFile.hpp"
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
#include "ImageProperties.hpp"
#include "LuaProxies.hpp"
#include "CPU.hpp"
#include "DebugRAM.hpp"
#include "BaseRenderer.hpp"
#include "IInputSource.hpp"
#include "ISystemDriver.hpp"
#include "VGMWriter.hpp"
#include "TraceHelper.hpp"


Manager::Manager() : mUI{ *this },
mLua{},
mDoReset{ false },
mDebugger{},
mProcessThreads{},
mJoinThreads{},
mRenderThread{},
mAudioThread{},
mRenderingTime{},
mScriptDebuggerEscapes{ std::make_shared<ScriptDebuggerEscapes>() },
mImageProperties{},
mRenderer{},
mDebugWindows{}
{
  mDebugger( RunMode::RUN );
  mAudioOut = std::make_shared<WinAudioOut>();
  mComLynxWire = std::make_shared<ComLynxWire>();

  mRenderThread = std::thread{ [this]
  {
    while ( !mJoinThreads.load() )
    {
      if ( mProcessThreads.load() )
      {
        auto renderingTime = mRenderer->render( mUI );
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
          if ( mAudioOut->wait() )
          {
            auto runMode = mDebugger.mRunMode.load();
            auto cpuBreakType = mAudioOut->fillBuffer( mInstance, renderingTime, runMode );
            if ( cpuBreakType != CpuBreakType::NEXT )
            {
              mDebugger.mRunMode.store( RunMode::PAUSE );
            }
          }
          mSystemDriver->setPaused( mDebugger.mRunMode.load() != RunMode::RUN );
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
  if ( mDoReset )
    reset();
  mDoReset = false;
}

void Manager::initialize( std::shared_ptr<ISystemDriver> systemDriver )
{
  assert( !mSystemDriver );

  mDebugWindows.cpuEditor.setManager( this );
  mDebugWindows.memoryEditor.setManager( this );
  mDebugWindows.watchEditor.setManager( this );
  mDebugWindows.disasmEditor.setManager( this );
  mDebugWindows.breakpointEditor.setManager( this );

  mSystemDriver = std::move( systemDriver );
  mRenderer = mSystemDriver->baseRenderer();
  mExtendedRenderer = mSystemDriver->extendedRenderer();

  mSystemDriver->registerDropFiles( std::bind( &Manager::handleFileDrop, this, std::placeholders::_1 ) );
  mSystemDriver->registerUpdate( std::bind( &Manager::update, this ) );
}

int Manager::win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  assert( mSystemDriver );
  return mSystemDriver->wndProcHandler( hWnd, msg, wParam, lParam );
}

IUserInput& Manager::userInput() const
{
  return *mSystemDriver->userInput();
}

Manager::~Manager()
{
  stopThreads();
}

void Manager::quit()
{
  mSystemDriver->quit();
}

void Manager::updateDebugWindows()
{

  if ( !mInstance || !mExtendedRenderer )
    return;

  if ( !mDebugger.isDebugMode() )
  {
    mDebugWindows.mainScreenView.reset();
    return;
  }

  std::unique_lock<std::mutex> l = mDebugger.lockMutex();

  if ( !mDebugWindows.mainScreenView )
  {
    mDebugWindows.mainScreenView = mExtendedRenderer->makeMainScreenView();
  }

  auto svs = mDebugger.screenViews();
  auto& csvs = mDebugWindows.customScreenViews;
  //removing elements in csvs that are not in svs 
  auto ret = std::ranges::remove_if( csvs, [&] ( int id ) { return std::ranges::find( svs, id, &ScreenView::id ) == svs.end(); }, [] ( auto const& p ) { return p.first; } );
  csvs.erase( ret.begin(), ret.end() );
  //add missing elements to csvs that are in svs
  for ( auto const& sv : svs )
  {
    if ( std::ranges::find( csvs, sv.id, [] ( auto const& p ) { return p.first; } ) == csvs.end() )
    {
      csvs.emplace_back( sv.id, mExtendedRenderer->makeCustomScreenView() );
    }
  }

  auto& cpu = mInstance->debugCPU();

  if ( mDebugger.isHistoryVisualized() )
  {
    auto& hisVis = mDebugger.historyVisualizer();
    cpu.copyHistory( std::span<char>( (char*)hisVis.data.data(), hisVis.data.size() ) );

    if ( !mDebugWindows.historyBoard )
    {
      mDebugWindows.historyBoard = mExtendedRenderer->makeBoard( hisVis.columns, hisVis.rows );
    }
  }
  else if ( !mDebugWindows.historyBoard )
  {
    mDebugWindows.historyBoard.reset();
  }
}

BoardRendering Manager::renderHistoryWindow()
{
  if ( mDebugger.isHistoryVisualized() && mDebugWindows.historyBoard )
  {
    auto win = mDebugger.historyVisualizer();
    auto tex = mDebugWindows.historyBoard->render( std::span<uint8_t const>{ win.data.data(), win.data.size() } );
    return { true, tex, 8.0f * win.columns , 16.0f * win.rows };
  }
  else
  {
    return { mDebugger.isHistoryVisualized() };
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
  mLua.new_usertype<CPUProxy>( "CPU", sol::meta_function::index, &CPUProxy::get, sol::meta_function::new_index, &CPUProxy::set );

  mLua["ram"] = std::make_unique<RamProxy>( *this );
  mLua["rom"] = std::make_unique<RomProxy>( *this );
  mLua["mikey"] = std::make_unique<MikeyProxy>( *this );
  mLua["suzy"] = std::make_unique<SuzyProxy>( *this );
  mLua["cpu"] = std::make_unique<CPUProxy>( *this );

  mLua["Encoder"] = [this] ( sol::table const& tab )
  {
    if ( !mExtendedRenderer )
      throw Ex{} << "Encoder not available";

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

    mEncoder = std::shared_ptr<IEncoder>( s_createEncoder( path.string().c_str(), vbitrate, abitrate, SCREEN_WIDTH * vscale, SCREEN_HEIGHT * vscale ), s_disposeEncoder );

    mExtendedRenderer->setEncoder( mEncoder );
    mAudioOut->setEncoder( mEncoder );
  };

  mLua["WavOut"] = [this] ( sol::table const& tab )
  {
    std::filesystem::path path;
    if ( sol::optional<std::string> opt = tab["path"] )
      path = *opt;
    else throw Ex{} << "path = \"path/to/file.wav\" required";

    mAudioOut->setWavOut( std::move( path ) );
  };

  mLua["vgmDump"] = [this] ( sol::table const& tab )
  {
    if ( sol::optional<std::string> opt = tab["path"] )
    {
      if ( mInstance )
        mInstance->setVGMWriter( *opt );
    }
    else throw Ex{} << "path = \"path/to/file.vgm\" required";
  };

  mLua["traceCurrent"] = [this] ()
  {
    if ( mInstance )
    {
      mInstance->debugCPU().toggleTrace( true );
    }
  };

  mLua["traceOn"] = [this] ()
  {
    if ( mInstance )
    {
      mInstance->debugCPU().enableTrace();
    }
  };
  mLua["traceOf"] = [this] ()
  {
    if ( mInstance )
    {
      mInstance->debugCPU().disableTrace();
    }
  };

  mLua["traceNextCount"] = [this]( int value )
  {
    if ( mInstance )
    {
      mInstance->debugCPU().traceNextCount( value );
    }
  };

  mLua.set_function("add_watch", [this] (std::string label, uint16_t addr, std::string datatype )
    {
      mDebugWindows.watchEditor.addWatch( label.c_str(), datatype.c_str(), addr );
    } );

  mLua.set_function( "del_watch", [this] ( std::string label )
    {
      mDebugWindows.watchEditor.deleteWatch( label.c_str() );
    } );

  mLua.set_function( "set_label", [this] ( uint16_t addr, std::string label )
    {
      if ( mInstance )
      {
        mInstance->getTraceHelper()->updateLabel( addr, label.c_str() );
      }
    } );

  auto trap = [this] ()
  {
    if ( mInstance )
    {
      mInstance->debugCPU().breakFromTrap();
    }
  };

  mLua["trap"] = trap;
  mLua["brk"] = trap;

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
  std::unique_lock<std::mutex> l = mDebugger.lockMutex();
  mProcessThreads.store( false );
  //TODO wait for threads to stop.
  mInstance.reset();

  if ( auto input = computeInputFile() )
  {
    mInstance = std::make_shared<Core>( *mImageProperties, mComLynxWire, mRenderer->getVideoSink(), mSystemDriver->userInput(),
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
    mInstance->debugCPU().breakOnBrk( mDebugger.isBreakOnBrk() );
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

  mDebugger( mDebugger.isDebugMode() ? RunMode::PAUSE : RunMode::RUN );
}

void Manager::updateRotation()
{
  mSystemDriver->updateRotation( mImageProperties->getRotation() );
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

void Manager::handleFileDrop( std::filesystem::path path )
{
  if ( !path.empty() )
    mArg = path;
  mDoReset = true;

  mSystemDriver->setImageName( path.filename().wstring() );
}

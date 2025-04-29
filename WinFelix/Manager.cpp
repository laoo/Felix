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
#include "ConfigProvider.hpp"
#include "SysConfig.hpp"
#include "ScriptDebuggerEscapes.hpp"
#include "UserInput.hpp"
#include "ImageROM.hpp"
#include "ImageProperties.hpp"
#include "LuaProxies.hpp"
#include "CPU.hpp"
#include "DebugRAM.hpp"
#include "Renderer.hpp"
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
mThreadsWaiting{},
mRenderThread{},
mAudioThread{},
mRenderingTime{},
mScriptDebuggerEscapes{},
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
        mThreadsWaiting.fetch_add( 1 );
        do
        {
          std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
        while ( !mProcessThreads.load() && !mJoinThreads.load() );
        mThreadsWaiting.fetch_sub( 1 );
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
          mThreadsWaiting.fetch_add( 1 );
          do
          {
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
          }
          while ( !mProcessThreads.load() && !mJoinThreads.load() );
          mThreadsWaiting.fetch_sub( 1 );
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
    machineReset();
  mDoReset = false;
}

void Manager::initialize( std::shared_ptr<ISystemDriver> systemDriver )
{
  assert( !mSystemDriver );

  mDebugWindows.cpuEditor.setManager( this );
  mDebugWindows.memoryEditor.setManager( this );
  mDebugWindows.disasmEditor.setManager( this );

  mSystemDriver = std::move( systemDriver );
  mRenderer = mSystemDriver->renderer();

  mSystemDriver->registerDropFiles( std::bind( &Manager::handleFileDrop, this, std::placeholders::_1 ) );
  mSystemDriver->registerUpdate( std::bind( &Manager::update, this ) );
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
  if ( !mInstance )
    return;

  if ( !mDebugger.isDebugMode() )
    return;

  std::unique_lock<std::mutex> l{ mDebugger.lockMutex() };

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
      csvs.emplace_back( sv.id, mRenderer->makeCustomScreenView() );
    }
  }

  auto& cpu = mInstance->debugCPU();
}

void Manager::processLua( std::filesystem::path const& path )
{
  auto luaPath = path;
  auto labPath = path;

  luaPath.replace_extension( path.extension().string() + ".lua" );
  labPath.replace_extension( ".lab" );

  if ( std::filesystem::exists( labPath ) )
  {
    mSymbols = std::make_unique<SymbolSource>( labPath );
  }
  else
  {
    mSymbols = std::make_unique<SymbolSource>();
  }

  if ( !std::filesystem::exists( luaPath ) )
    return;

  mLua = sol::state{};
  mLua.open_libraries( sol::lib::base, sol::lib::io, sol::lib::string );

  mLua.new_usertype<TrapProxy>( "TRAP", sol::meta_function::new_index, &TrapProxy::set );
  mLua.new_usertype<RamProxy>( "RAM", sol::meta_function::index, &RamProxy::get, sol::meta_function::new_index, &RamProxy::set );
  mLua.new_usertype<RomProxy>( "ROM", sol::meta_function::index, &RomProxy::get, sol::meta_function::new_index, &RomProxy::set );
  mLua.new_usertype<MikeyProxy>( "MIKEY", sol::meta_function::index, &MikeyProxy::get, sol::meta_function::new_index, &MikeyProxy::set );
  mLua.new_usertype<SuzyProxy>( "SUZY", sol::meta_function::index, &SuzyProxy::get, sol::meta_function::new_index, &SuzyProxy::set );
  mLua.new_usertype<CPUProxy>( "CPU", sol::meta_function::index, &CPUProxy::get, sol::meta_function::new_index, &CPUProxy::set );
  mLua.new_usertype<SymbolProxy>( "SYMBOL", sol::meta_function::index, &SymbolProxy::get );

  mLua["ram"] = std::make_unique<RamProxy>( *this );
  mLua["rom"] = std::make_unique<RomProxy>( *this );
  mLua["mikey"] = std::make_unique<MikeyProxy>( *this );
  mLua["suzy"] = std::make_unique<SuzyProxy>( *this );
  mLua["cpu"] = std::make_unique<CPUProxy>( *this );
  mLua["symbol"] = std::make_unique<SymbolProxy>( *this );

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

  mLua["dumpSprites"] = [this]( sol::table const& tab )
  {
    if ( sol::optional<std::string> opt = tab["path"] )
    {
      if ( mInstance )
        mInstance->dumpSprites( *opt );
    }
    else
    {
      if ( mInstance )
        mInstance->dumpSprites( {} );
    }
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
  mLua["traceOff"] = [this] ()
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

  mLua.set_function( "setLabel", [this] ( uint16_t addr, std::string label )
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

  auto monitFun = [this]( std::string label, Monitor::Entry::Type type, int size )
  {
    if ( auto opt = mSymbols->symbol( label ) )
    {
      mMonitor.addEntry( { type, label, *opt, (uint16_t)size } );
    }
    else
    {
      mMonitor.addEntry( {} );
    }
  };

  mLua["monitHByte"] = [=,this]( std::string label ) { monitFun( std::move( label ), Monitor::Entry::Type::HEX, 1 ); };
  mLua["monitUByte"] = [=,this]( std::string label ) { monitFun( std::move( label ), Monitor::Entry::Type::UNSIGNED, 1 ); };
  mLua["monitSByte"] = [=,this]( std::string label ) { monitFun( std::move( label ), Monitor::Entry::Type::SIGNED, 1 ); };
  mLua["monitHWord"] = [=,this]( std::string label ) { monitFun( std::move( label ), Monitor::Entry::Type::HEX, 2 ); };
  mLua["monitUWord"] = [=,this]( std::string label ) { monitFun( std::move( label ), Monitor::Entry::Type::UNSIGNED, 2 ); };
  mLua["monitSWord"] = [=,this]( std::string label ) { monitFun( std::move( label ), Monitor::Entry::Type::SIGNED, 2 ); };
  mLua["monitHLong"] = [=, this]( std::string label ) { monitFun( std::move( label ), Monitor::Entry::Type::HEX, 4 ); };
  mLua["monitULong"] = [=, this]( std::string label ) { monitFun( std::move( label ), Monitor::Entry::Type::UNSIGNED, 4 ); };
  mLua["monitSLong"] = [=, this]( std::string label ) { monitFun( std::move( label ), Monitor::Entry::Type::SIGNED, 4 ); };
  mLua["monitHex"] = [=, this]( std::string label, int size ) { monitFun( std::move( label ), Monitor::Entry::Type::HEX, size ); };


  mLua.script_file( luaPath.string() );

  if ( sol::optional<std::string> opt = mLua["log"] )
  {
    std::filesystem::path path{ *opt };
    if ( path.is_absolute() )
    {
      mLogPath = path;
    }
    else
    {
      mLogPath = luaPath.parent_path() / path;
    }
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

void Manager::machineReset()
{
  mProcessThreads.store( false );
  while ( mThreadsWaiting.load() != 2 )
  {
    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
  }
  std::unique_lock<std::mutex> l{ mDebugger.lockMutex() };
  mInstance.reset();

  mScriptDebuggerEscapes = std::make_shared<ScriptDebuggerEscapes>();

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

#include "pch.hpp"
#include "Debugger.hpp"
#include "ConfigProvider.hpp"
#include "SysConfig.hpp"

/* Debug Window Sizes */
#define DISASM_WIDTH   40
#define DISASM_HEIGHT  16
#define CPU_WIDTH      14
#define CPU_HEIGHT      3
#define HISTORY_WIDTH  64
#define HISTORY_HEIGHT 16

Debugger::Debugger() : mMutex{},
mDebugMode{},
visualizeCPU{},
visualizeMemory{},
visualizeDisasm{},
mVisualizeHistory{},
mDebugModeOnBreak{},
mNormalModeOnRun{},
mHistoryVisualizer{ HISTORY_WIDTH, HISTORY_HEIGHT },
mRunMode{ RunMode::RUN }
{
  auto sysConfig = gConfigProvider.sysConfig();

  mDebugMode = sysConfig->debugMode;
  visualizeCPU = sysConfig->visualizeCPU;
  visualizeMemory = sysConfig->visualizeMemory;
  visualizeWatch = sysConfig->visualizeWatch;
  visualizeBreakpoint = sysConfig->visualizeBreakpoint;
  visualizeDisasm = sysConfig->visualizeDisasm;
  mVisualizeHistory = sysConfig->visualizeHistory;
  mDebugModeOnBreak = sysConfig->debugModeOnBreak;
  mNormalModeOnRun = sysConfig->normalModeOnRun;
  mBreakOnBrk = sysConfig->breakOnBrk;
  showMonitor = sysConfig->showMonitor;
  for ( auto const& sv : sysConfig->screenViews )
  {
    mScreenViews.emplace_back( sv.id, (ScreenViewType)sv.type, (uint16_t)sv.customAddress, sv.safePalette );
  }
}

Debugger::~Debugger()
{
  auto sysConfig = gConfigProvider.sysConfig();

  sysConfig->debugMode = mDebugMode;
  sysConfig->visualizeCPU = visualizeCPU;
  sysConfig->visualizeMemory = visualizeMemory;
  sysConfig->visualizeWatch = visualizeWatch;
  sysConfig->visualizeBreakpoint = visualizeBreakpoint;
  sysConfig->visualizeDisasm = visualizeDisasm;
  sysConfig->visualizeHistory = mVisualizeHistory;
  sysConfig->debugModeOnBreak = mDebugModeOnBreak;
  sysConfig->normalModeOnRun = mNormalModeOnRun;
  sysConfig->breakOnBrk = mBreakOnBrk;
  sysConfig->showMonitor = showMonitor;
  sysConfig->screenViews.clear();
  for ( auto const& sv : mScreenViews )
  {
    sysConfig->screenViews.emplace_back( sv.id, (int)sv.type, (int)sv.customAddress, sv.safePalette );
  }
}

void Debugger::operator()( RunMode mode )
{
  if ( mode == RunMode::RUN && mNormalModeOnRun )
  {
    debugMode( false );
  }
  if ( mode == RunMode::PAUSE && mDebugModeOnBreak )
  {
    debugMode( true );
  }

  mRunMode.store( mode );
}

bool Debugger::isPaused() const
{
  return mRunMode.load() == RunMode::PAUSE;
}

bool Debugger::isDebugMode() const
{
  return mDebugMode;
}

bool Debugger::isHistoryVisualized() const
{
  return mVisualizeHistory;
}

bool Debugger::isBreakOnBrk() const
{
  return mBreakOnBrk;
}

std::span<ScreenView> Debugger::screenViews()
{
  if ( mScreenViews.empty() )
  {
    return {};
  }
  else
  {
    return std::span<ScreenView>{ mScreenViews.data(), mScreenViews.size() };
  }
}

void Debugger::visualizeHistory( bool value )
{
  mVisualizeHistory = value;
}

void Debugger::debugMode( bool value )
{
  mDebugMode = value;
}

bool Debugger::debugModeOnBreak() const
{
  return mDebugModeOnBreak;
}

void Debugger::debugModeOnBreak( bool value )
{
  mDebugModeOnBreak = value;
}

bool Debugger::normalModeOnRun() const
{
  return mNormalModeOnRun;
}

void Debugger::normalModeOnRun( bool value )
{
  mNormalModeOnRun = value;
}

void Debugger::breakOnBrk( bool value )
{
  mBreakOnBrk = value;
}

void Debugger::newScreenView()
{
  int minId = 0;

  for ( ;; )
  {
    auto it = std::ranges::find( mScreenViews, minId, &ScreenView::id );
    if ( it == mScreenViews.cend() )
      break;

    minId += 1;
  }

  mScreenViews.emplace_back( minId, ScreenViewType::DISPADR );
}

void Debugger::delScreenView( int id )
{
  std::erase_if( mScreenViews, [=] ( auto const& sv )
    {
      return sv.id == id;
    } );
}

void Debugger::togglePause()
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

DebugWindow& Debugger::historyVisualizer()
{
  return mHistoryVisualizer;
}

std::unique_lock<std::mutex> Debugger::lockMutex() const
{
  return std::unique_lock<std::mutex>{ mMutex };
}

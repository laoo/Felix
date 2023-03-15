#include "pch.hpp"
#include "Debugger.hpp"
#include "BreakpointEditor.hpp"
#include "CPUEditor.hpp"
#include "DisasmEditor.h"
#include "MemEditor.hpp"
#include "WatchEditor.hpp"
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
mVisualizedEditors{},
mVisualizeHistory{},
mDebugModeOnBreak{},
mNormalModeOnRun{},
mHistoryVisualizer{ HISTORY_WIDTH, HISTORY_HEIGHT },
mRunMode{ RunMode::RUN }
{
  auto sysConfig = gConfigProvider.sysConfig();

  mDebugMode = sysConfig->debugMode;
  mVisualizeHistory = sysConfig->visualizeHistory;
  mDebugModeOnBreak = sysConfig->debugModeOnBreak;
  mNormalModeOnRun = sysConfig->normalModeOnRun;
  mBreakOnBrk = sysConfig->breakOnBrk;
  for ( auto const& sv : sysConfig->screenViews )
  {
    mScreenViews.emplace_back( sv.id, (ScreenViewType)sv.type, (uint16_t)sv.customAddress, sv.safePalette );
  }
}

Debugger::~Debugger()
{
  auto sysConfig = gConfigProvider.sysConfig();

  sysConfig->debugMode = mDebugMode;
  sysConfig->visualizeHistory = mVisualizeHistory;
  sysConfig->debugModeOnBreak = mDebugModeOnBreak;
  sysConfig->normalModeOnRun = mNormalModeOnRun;
  sysConfig->breakOnBrk = mBreakOnBrk;
  sysConfig->screenViews.clear();
  for ( auto const& sv : mScreenViews )
  {
    sysConfig->screenViews.emplace_back( sv.id, (int)sv.type, (int)sv.customAddress, sv.safePalette );
  }
}

void Debugger::initialize( Manager* manager )
{
  auto sysConfig = gConfigProvider.sysConfig();

  for ( int i = 0; i < sysConfig->debugOptions.visualizeBreakCount; ++i )
  {
    addDebugControl( IEditor::Breakpoint, manager, true );
  }
  for (int i = 0; i < sysConfig->debugOptions.visualizeCPUCount; ++i)
  {
    addDebugControl( IEditor::CPU, manager, true );
  }
  for (int i = 0; i < sysConfig->debugOptions.visualizeDisasmCount; ++i)
  {
    addDebugControl( IEditor::Disasm, manager, true );
  }
  for (int i = 0; i < sysConfig->debugOptions.visualizeMemCount; ++i)
  {
    addDebugControl( IEditor::Memory, manager, true );
  }
  for (int i = 0; i < sysConfig->debugOptions.visualizeWatchCount; ++i)
  {
    addDebugControl( IEditor::Watch, manager, true );
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

void Debugger::addDebugControl( IEditor::EditorType type, Manager *manager, bool initialize )
{
  DebugControl ctrl;
  auto sysConfig = gConfigProvider.sysConfig();

  switch (type)
  {
  case IEditor::EditorType::Breakpoint:
    ctrl.editor = std::static_pointer_cast<IEditor>(std::make_shared<BreakpointEditor>());
    if (!initialize)
    {
      sysConfig->debugOptions.visualizeBreakCount++;
    }
    break;
  case IEditor::EditorType::CPU:
    ctrl.editor = std::static_pointer_cast<IEditor>(std::make_shared<CPUEditor>());
    if (!initialize)
    {
      sysConfig->debugOptions.visualizeCPUCount++;
    }
    break;
  case IEditor::EditorType::Disasm:
    ctrl.editor = std::static_pointer_cast<IEditor>(std::make_shared<DisasmEditor>());
    if (!initialize)
    {
      sysConfig->debugOptions.visualizeDisasmCount++;
    }
    break;
  case IEditor::EditorType::Memory:
    ctrl.editor = std::static_pointer_cast<IEditor>(std::make_shared<MemEditor>());
    if (!initialize)
    {
      sysConfig->debugOptions.visualizeMemCount++;
    }
    break;
  case IEditor::EditorType::Watch:
    ctrl.editor = std::static_pointer_cast<IEditor>(std::make_shared<WatchEditor>());
    if (!initialize)
    {
      sysConfig->debugOptions.visualizeWatchCount++;
    }
    break;
  }

  ctrl.type = type;
  ctrl.label = EditorDefinitions[type].label;
  ctrl.editor->setManager( manager );

  int count = (int)getDebugControls( type ).size();

  if (count > 0)
  {
    ctrl.label += " " + std::to_string(count + 1);
  }

  mVisualizedEditors.push_back( ctrl );
}

void Debugger::deleteDebugControl( std::string label )
{
  auto found = std::find_if( mVisualizedEditors.begin(), mVisualizedEditors.end(), [label]( DebugControl ed ) { return label == ed.label; } );

  if (found == mVisualizedEditors.end())
  {
    return;
  }
    
  auto sysConfig = gConfigProvider.sysConfig();

  switch (found->type)
  {
  case IEditor::EditorType::Breakpoint:
    sysConfig->debugOptions.visualizeBreakCount--;
    break;
  case IEditor::EditorType::CPU:
    sysConfig->debugOptions.visualizeCPUCount--;
    break;
  case IEditor::EditorType::Disasm:
    sysConfig->debugOptions.visualizeDisasmCount--;
    break;
  case IEditor::EditorType::Memory:
    sysConfig->debugOptions.visualizeMemCount--;
    break;
  case IEditor::EditorType::Watch:
    sysConfig->debugOptions.visualizeWatchCount--;
    break;
  }

  std::erase_if( mVisualizedEditors, [label]( DebugControl ed ) { return label == ed.label; } );
}

std::vector<DebugControl> Debugger::getDebugControls( IEditor::EditorType type )
{
  std::vector<DebugControl> results{};

  std::copy_if( 
    mVisualizedEditors.begin(), mVisualizedEditors.end(), 
    std::back_inserter ( results ),
    [type]( const DebugControl ed ) { return type == ed.type; } );

  return results;
}

std::vector<DebugControl> Debugger::getDebugControls()
{
  std::vector<DebugControl> results{ mVisualizedEditors };
  return results;
}
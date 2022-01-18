#pragma once

#include "Utility.hpp"
#include "BaseRenderer.hpp"

struct DebugWindow
{
  DebugWindow( int id, int columns, int rows ) : id{ id }, columns{ columns }, rows{ rows }
  {
    data.resize( columns* rows );
  }

  int id;
  int columns;
  int rows;
  std::vector<uint8_t> data;
};

struct ScreenView
{
  int id;
  ScreenViewType type;
  uint16_t customAddress;
  bool safePalette;
};

class Debugger
{
public:
  Debugger();
  ~Debugger();

  void operator()( RunMode mode );

  bool isPaused() const;
  bool isDebugMode() const;
  bool isCPUVisualized() const;
  bool isDisasmVisualized() const;
  bool isHistoryVisualized() const;
  bool isBreakOnBrk() const;

  std::span<ScreenView> screenViews();

  void visualizeCPU( bool value );
  void visualizeDisasm( bool value );
  void visualizeHistory( bool value );
  void debugMode( bool value );
  bool debugModeOnBreak() const;
  void debugModeOnBreak( bool value );
  bool normalModeOnRun() const;
  void normalModeOnRun( bool value );
  void breakOnBrk( bool value );
  void newScreenView();
  void delScreenView( int id );

  void togglePause();

  DebugWindow& cpuVisualizer();
  DebugWindow& disasmVisualizer();
  DebugWindow& historyVisualizer();

  void mainScreenView( std::shared_ptr<IScreenView> mainDebugView );
  std::shared_ptr<IScreenView> mainScreenView() const;

  mutable std::mutex mutex;

private:
  std::vector<ScreenView> mScreenViews;
  std::shared_ptr<IScreenView> mMainScreenView;
  DebugWindow mCpuVisualizer;
  DebugWindow mDisasmVisualizer;
  DebugWindow mHistoryVisualizer;
  bool mDebugMode;
  bool mVisualizeCPU;
  bool mVisualizeDisasm;
  bool mVisualizeHistory;
  bool mDebugModeOnBreak;
  bool mNormalModeOnRun;
  bool mBreakOnBrk;
  friend class Manager;
  std::atomic<RunMode> mRunMode;

};

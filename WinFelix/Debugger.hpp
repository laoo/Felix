#pragma once

#include "Utility.hpp"
#include "BaseRenderer.hpp"

struct DebugWindow
{
  DebugWindow( int columns, int rows ) : columns{ columns }, rows{ rows }
  {
    data.resize( columns * rows );
  }

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
  bool isHistoryVisualized() const;
  bool isBreakOnBrk() const;

  std::span<ScreenView> screenViews();

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

  DebugWindow& historyVisualizer();

  std::unique_lock<std::mutex> lockMutex() const;

  bool visualizeCPU;
  bool visualizeMemory;
  bool visualizeWatch;
  bool visualizeBreakpoint;
  bool visualizeDisasm;
  bool showMonitor;

private:
  mutable std::mutex mMutex;
  std::vector<ScreenView> mScreenViews;
  DebugWindow mHistoryVisualizer;
  bool mDebugMode;  
  bool mVisualizeHistory;
  bool mDebugModeOnBreak;
  bool mNormalModeOnRun;
  bool mBreakOnBrk;
  friend class Manager;
  std::atomic<RunMode> mRunMode;

};

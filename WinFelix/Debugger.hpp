#pragma once

#include "Utility.hpp"
#include "Renderer.hpp"

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
  bool isBreakOnBrk() const;

  std::span<ScreenView> screenViews();

  void debugMode( bool value );
  bool debugModeOnBreak() const;
  void debugModeOnBreak( bool value );
  bool normalModeOnRun() const;
  void normalModeOnRun( bool value );
  void breakOnBrk( bool value );
  void newScreenView();
  void delScreenView( int id );

  void togglePause();

  std::unique_lock<std::mutex> lockMutex() const;

  bool visualizeCPU;
  bool visualizeMemory;
  bool visualizeWatch;
  bool visualizeBreakpoint;
  bool visualizeDisasm;

private:
  mutable std::mutex mMutex;
  std::vector<ScreenView> mScreenViews;
  bool mDebugMode;  
  bool mDebugModeOnBreak;
  bool mNormalModeOnRun;
  bool mBreakOnBrk;
  friend class Manager;
  std::atomic<RunMode> mRunMode;

};

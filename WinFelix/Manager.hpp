#pragma once

#include "IInputSource.hpp"
class WinRenderer;
class WinAudioOut;
class ComLynxWire;
class Core;

class Manager
{
public:
  Manager();
  ~Manager();

  void doArgs( std::vector<std::wstring> args );
  void initialize( HWND hWnd );
  int win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

  void drawGui( int left, int top, int right, int bottom );

  void horizontalView( bool horizontal );

  bool doRun() const;
  bool horizontalView() const;

  void processKeys();

  std::shared_ptr<IInputSource> getInputSource( int instance );

private:
  void stopThreads();

private:

  struct InputSource : public IInputSource, public KeyInput
  {
    InputSource();
    ~InputSource() override = default;

    KeyInput getInput() const override;
  };

  bool mEmulationRunning;
  bool mHorizontalView;

  std::array<std::shared_ptr<InputSource>, 2> mIntputSources;
  std::atomic<bool> mProcessThreads;
  int mInstancesCount;
  std::thread mRenderThread;
  std::thread mAudioThread;
  std::shared_ptr<WinRenderer> mRenderer;
  std::shared_ptr<WinAudioOut> mAudioOut;
  std::shared_ptr<ComLynxWire> mComLynxWire;
  std::vector<std::shared_ptr<Core>> mInstances;


};

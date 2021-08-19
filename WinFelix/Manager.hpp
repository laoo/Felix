#pragma once

#include "IInputSource.hpp"
#include "IEscape.hpp"
#include "WinConfig.hpp"


class WinRenderer;
class WinAudioOut;
class ComLynxWire;
class Core;
class Monitor;
class SymbolSource;
class InputFile;
class IEncoder;

class Manager
{
public:
  Manager();
  ~Manager();

  void update();
  void reset();
  void doArgs( std::vector<std::wstring> args );
  WinConfig const& getWinConfig();
  void initialize( HWND hWnd );
  int win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

  void drawGui( int left, int top, int right, int bottom );

  void horizontalView( bool horizontal );

  bool doRun() const;
  bool horizontalView() const;

  std::shared_ptr<IInputSource> getInputSource( int instance );

private:
  void processLua( std::filesystem::path const& path, std::vector<InputFile>& inputs );
  void stopThreads();
  void handleFileDrop( HDROP hDrop );
  bool handleCopyData( COPYDATASTRUCT const* copy );
  void processKeys();
  static std::filesystem::path getAppDataFolder();
private:

  struct InputSource : public IInputSource, public KeyInput
  {
    InputSource();
    ~InputSource() override = default;

    KeyInput getInput() const override;
  };

  struct Escape : IEscape
  {
    Manager & manager;
    Escape( Manager & manager ) : manager{ manager } {}
    ~Escape() override = default;
    void call( uint8_t data, IAccessor & accessor ) override;
  };

  bool mEmulationRunning;
  bool mHorizontalView;

  bool mDoUpdate;

  std::array<std::shared_ptr<InputSource>, 2> mIntputSources;
  std::atomic<bool> mProcessThreads;
  int mInstancesCount;
  HMODULE mEncoderMod;
  std::thread mRenderThread;
  std::thread mAudioThread;
  std::shared_ptr<WinRenderer> mRenderer;
  std::shared_ptr<WinAudioOut> mAudioOut;
  std::shared_ptr<ComLynxWire> mComLynxWire;
  std::unique_ptr<Monitor> mMonitor;
  std::shared_ptr<IEncoder> mEncoder;
  std::unique_ptr<SymbolSource> mSymbols;
  std::vector<std::shared_ptr<Core>> mInstances;
  std::vector<std::wstring> mArgs;
  std::filesystem::path mAppDataFolder;
  std::filesystem::path mLogPath;
  uint64_t mLogStartCycle;
  WinConfig mWinConfig;
  std::atomic_bool mPaused;
  sol::state mLua;
  std::vector<sol::function> mEscapes;
  sol::function mMonit;
  std::mutex mMutex;
};

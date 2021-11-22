#pragma once

#include "IInputSource.hpp"
#include "IEscape.hpp"

class WinRenderer;
class WinAudioOut;
class ComLynxWire;
class Core;
class Monitor;
class SymbolSource;
class InputFile;
class IEncoder;
class WinImgui;
class ScriptDebugger;
struct ImGuiIO;

namespace ImGui
{
class FileBrowser;
}

class Manager
{
public:
  Manager();
  ~Manager();

  void update();
  void reset();
  void doArg( std::wstring arg );
  void initialize( HWND hWnd );
  int win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
  void setWinImgui( std::shared_ptr<WinImgui> winImgui );

  bool mainMenu( ImGuiIO& io );

  void drawGui( int left, int top, int right, int bottom );

  bool doRun() const;

private:
  std::optional<InputFile> processLua( std::filesystem::path const& path );
  std::optional<InputFile> computeInputFile();
  void stopThreads();
  void handleFileDrop( HDROP hDrop );
  bool handleCopyData( COPYDATASTRUCT const* copy );
  void processKeys();
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

  bool mDoUpdate;

  std::shared_ptr<InputSource> mIntputSource;
  std::atomic_bool mProcessThreads;
  std::atomic_bool mJoinThreads;
  std::atomic_bool mPaused;
  HMODULE mEncoderMod;
  std::thread mRenderThread;
  std::thread mAudioThread;
  std::shared_ptr<WinRenderer> mRenderer;
  std::shared_ptr<WinAudioOut> mAudioOut;
  std::shared_ptr<ComLynxWire> mComLynxWire;
  std::unique_ptr<Monitor> mMonitor;
  std::shared_ptr<IEncoder> mEncoder;
  std::unique_ptr<SymbolSource> mSymbols;
  std::shared_ptr<Core> mInstance;
  std::shared_ptr<ScriptDebugger> mScriptDebugger;
  std::wstring mArg;
  std::filesystem::path mLogPath;
  uint64_t mLogStartCycle;
  sol::state mLua;
  std::vector<sol::function> mEscapes;
  sol::function mMonit;
  std::mutex mMutex;
  int64_t mRenderingTime;
  bool mOpenMenu;
  std::unique_ptr<ImGui::FileBrowser> mFileBrowser;
};

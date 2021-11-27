#pragma once

#include "ScriptDebugger.hpp"

class WinRenderer;
class WinAudioOut;
class ComLynxWire;
class Core;
class SymbolSource;
class InputFile;
class IEncoder;
class WinImgui;
class ScriptDebuggerEscapes;
class UserInput;
class KeyNames;
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
private:

  struct TrapProxy
  {
    std::shared_ptr<ScriptDebuggerEscapes> scriptDebuggerEscapes;
    ScriptDebugger::Type type;

    static void set( TrapProxy & proxy, int idx, sol::function fun );
  };

  struct RamProxy
  {
    Manager& manager;

    sol::object get( sol::stack_object key, sol::this_state L );
    void set( sol::stack_object key, sol::stack_object value, sol::this_state );
  };

  struct RomProxy
  {
    Manager& manager;

    sol::object get( sol::stack_object key, sol::this_state L );
    void set( sol::stack_object key, sol::stack_object value, sol::this_state );
  };

  struct MikeyProxy
  {
    Manager& manager;

    sol::object get( sol::stack_object key, sol::this_state L );
    void set( sol::stack_object key, sol::stack_object value, sol::this_state );
  };

  struct SuzyProxy
  {
    Manager& manager;

    sol::object get( sol::stack_object key, sol::this_state L );
    void set( sol::stack_object key, sol::stack_object value, sol::this_state );
  };

  bool mEmulationRunning;

  bool mDoUpdate;

  sol::state mLua;
  std::atomic_bool mProcessThreads;
  std::atomic_bool mJoinThreads;
  std::atomic_bool mPaused;
  HMODULE mEncoderMod;
  std::thread mRenderThread;
  std::thread mAudioThread;
  std::shared_ptr<WinRenderer> mRenderer;
  std::shared_ptr<WinAudioOut> mAudioOut;
  std::shared_ptr<ComLynxWire> mComLynxWire;
  std::shared_ptr<IEncoder> mEncoder;
  std::unique_ptr<SymbolSource> mSymbols;
  std::shared_ptr<Core> mInstance;
  std::shared_ptr<ScriptDebuggerEscapes> mScriptDebuggerEscapes;
  std::shared_ptr<UserInput> mIntputSource;
  std::shared_ptr<KeyNames> mKeyNames;
  std::wstring mArg;
  std::filesystem::path mLogPath;
  uint64_t mLogStartCycle;
  std::mutex mMutex;
  int64_t mRenderingTime;
  bool mOpenMenu;
  std::unique_ptr<ImGui::FileBrowser> mFileBrowser;
};

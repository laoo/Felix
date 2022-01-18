#pragma once

#include "ScriptDebugger.hpp"
#include "Utility.hpp"
#include "BaseRenderer.hpp"
#include "UI.hpp"
#include "Debugger.hpp"

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
class ImageProperties;
class ImageROM;
struct ImGuiIO;

class Manager
{
public:
  Manager();
  ~Manager();

  void update();
  void reset();
  void updateRotation();
  void doArg( std::wstring arg );
  void initialize( HWND hWnd );
  int win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

  void quit();


private:
  void processLua( std::filesystem::path const& path );
  std::optional<InputFile> computeInputFile();
  void stopThreads();
  void handleFileDrop( HDROP hDrop );
  bool handleCopyData( COPYDATASTRUCT const* copy );

  void updateDebugWindows();

  static std::shared_ptr<ImageROM const> getOptionalBootROM();
private:

  friend struct RamProxy;
  friend struct RomProxy;
  friend struct MikeyProxy;
  friend struct SuzyProxy;
  friend struct CPUProxy;
  friend class UI;

  bool mDoReset;

  Debugger mDebugger;

  struct DebugWindows
  {
    std::shared_ptr<IScreenView> mainScreenView;
    std::vector<std::pair<int, std::shared_ptr<ICustomScreenView>>> customScreenViews;
  } mDebugWindows;

  UI mUI;
  sol::state mLua;
  std::atomic_bool mProcessThreads;
  std::atomic_bool mJoinThreads;
  HMODULE mEncoderMod;
  std::thread mRenderThread;
  std::thread mAudioThread;
  std::shared_ptr<BaseRenderer> mRenderer;
  std::shared_ptr<IExtendedRenderer> mExtendedRenderer;
  std::shared_ptr<WinAudioOut> mAudioOut;
  std::shared_ptr<ComLynxWire> mComLynxWire;
  std::shared_ptr<IEncoder> mEncoder;
  std::unique_ptr<SymbolSource> mSymbols;
  std::shared_ptr<Core> mInstance;
  std::shared_ptr<ScriptDebuggerEscapes> mScriptDebuggerEscapes;
  std::shared_ptr<UserInput> mIntputSource;
  std::shared_ptr<ImageProperties> mImageProperties;
  std::filesystem::path mArg;
  std::filesystem::path mLogPath;
  std::mutex mMutex;
  int64_t mRenderingTime;
  HWND mhWnd;

};

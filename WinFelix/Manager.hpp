#pragma once

#include "ScriptDebugger.hpp"
#include "Utility.hpp"
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
class IBaseRenderer;
class IExtendedRenderer;
class ISystemDriver;

class Manager
{
public:
  Manager();
  ~Manager();

  void update();
  void reset();
  void updateRotation();
  void initialize( std::shared_ptr<ISystemDriver> systemDriver );
  int win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
  IUserInput & userInput() const;
  void quit();


private:
  void processLua( std::filesystem::path const& path );
  std::optional<InputFile> computeInputFile();
  void stopThreads();
  void handleFileDrop( std::filesystem::path path );

  void updateDebugWindows();
  BoardRendering renderCPUWindow();
  BoardRendering renderDisasmWindow();
  BoardRendering renderHistoryWindow();
  
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
    std::shared_ptr<IBoard> cpuBoard;
    std::shared_ptr<IBoard> disasmBoard;
    std::shared_ptr<IBoard> historyBoard;
  } mDebugWindows;

  UI mUI;
  sol::state mLua;
  std::atomic_bool mProcessThreads;
  std::atomic_bool mJoinThreads;
  HMODULE mEncoderMod;
  std::thread mRenderThread;
  std::thread mAudioThread;
  std::shared_ptr<ISystemDriver> mSystemDriver;
  std::shared_ptr<IBaseRenderer> mRenderer;
  std::shared_ptr<IExtendedRenderer> mExtendedRenderer;
  std::shared_ptr<WinAudioOut> mAudioOut;
  std::shared_ptr<ComLynxWire> mComLynxWire;
  std::shared_ptr<IEncoder> mEncoder;
  std::unique_ptr<SymbolSource> mSymbols;
  std::shared_ptr<Core> mInstance;
  std::shared_ptr<ScriptDebuggerEscapes> mScriptDebuggerEscapes;
  std::shared_ptr<ImageProperties> mImageProperties;
  std::filesystem::path mArg;
  std::filesystem::path mLogPath;
  std::mutex mMutex;
  int64_t mRenderingTime;
};

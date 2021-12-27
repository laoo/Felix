#pragma once

#include "ScriptDebugger.hpp"
#include "IInputSource.hpp"
#include "Utility.hpp"

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
class ImageProperties;
class ImageROM;
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
  void updateRotation();
  void doArg( std::wstring arg );
  void initialize( HWND hWnd );
  int win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

  void quit();

  void drawGui( int left, int top, int right, int bottom );

private:
  void processLua( std::filesystem::path const& path );
  std::optional<InputFile> computeInputFile();
  void stopThreads();
  void handleFileDrop( HDROP hDrop );
  bool handleCopyData( COPYDATASTRUCT const* copy );

  bool mainMenu( ImGuiIO& io );
  void configureKeyWindow( std::optional<KeyInput::Key>& keyToConfigure );
  void imagePropertiesWindow( bool init );
  void drawDebugWindows( ImGuiIO& io );
  void updateDebugWindows();

  static std::shared_ptr<ImageROM const> getOptionalBootROM();
private:

  friend struct RamProxy;
  friend struct RomProxy;
  friend struct MikeyProxy;
  friend struct SuzyProxy;
  friend struct CPUProxy;

  bool mDoUpdate;

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

  struct DebugWindows
  {
    std::mutex mutex;
    std::unique_ptr<DebugWindow> cpu;
    std::unique_ptr<DebugWindow> disasm;
    std::unique_ptr<DebugWindow> history;
  } mDebugWindows;

  void renderBoard( DebugWindow& win );

  sol::state mLua;
  std::atomic_bool mProcessThreads;
  std::atomic_bool mJoinThreads;
  std::atomic<RunMode> mRunMode;
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
  std::shared_ptr<ImageProperties> mImageProperties;
  std::filesystem::path mArg;
  std::filesystem::path mLogPath;
  std::mutex mMutex;
  int64_t mRenderingTime;
  bool mOpenMenu;
  std::unique_ptr<ImGui::FileBrowser> mFileBrowser;
  HWND mhWnd;
};

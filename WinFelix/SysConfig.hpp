#pragma once

struct SysConfig
{
  struct
  {
    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;
    int width = 960;
    int height = 630;
  } mainWindow;
  bool singleInstance = false;
  struct BootROM
  {
    bool useExternal = false;
    std::filesystem::path path{};
  } bootROM;
  struct KeyMapping
  {
    int pause = '2';
    int down = VK_DOWN;
    int up = VK_UP;
    int right = VK_RIGHT;
    int left = VK_LEFT;
    int option1 = '1';
    int option2 = '3';
    int inner = 'Z';
    int outer = 'X';
  } keyMapping;
  std::filesystem::path lastOpenDirectory{};
  bool debugMode;
  bool visualizeCPU;
  bool visualizeDisasm;
  struct DisasmyOptions
  {
    bool  FollowPC = true;
    bool  ShowLabelsInAddrCol = true;
  } disasmOptions;
  bool visualizeMemory;
  struct MemoryOptions
  {
    bool  OptShowOptions = true;
    bool  OptShowDataPreview;
    bool  OptShowHexII;
    bool  OptShowAscii;
    bool  OptGreyOutZeroes;
    bool  OptUpperCaseHex;
    int   OptMidColsCount;
    int   OptAddrDigitsCount;
    float OptFooterExtraHeight;
  } memoryOptions;
  bool visualizeWatch;
  bool visualizeBreakpoint;
  bool visualizeHistory;
  bool debugModeOnBreak;
  bool normalModeOnRun;
  bool breakOnBrk;
  struct ScreenView
  {
    int id;
    int type;
    int customAddress;
    int safePalette;
  };
  std::vector<ScreenView> screenViews;
  struct Audio
  {
    bool mute;
  } audio;

  static std::shared_ptr<SysConfig> load( std::filesystem::path path );
  void serialize( std::filesystem::path path );

private:
  void load( sol::state const& lua );
};

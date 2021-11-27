#pragma once

struct SysConfig
{
  bool singleInstance = false;
  struct Kernel
  {
    bool useExternal = false;
    std::filesystem::path path{};
  } kernel;
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

  static std::shared_ptr<SysConfig> load( std::filesystem::path path );
  void serialize( std::filesystem::path path );

private:
  void load( sol::state const& lua );
};
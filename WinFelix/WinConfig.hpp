#pragma once

struct WinConfig
{
  struct
  {
    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;
    int width = 960;
    int height = 630;
  } mainWindow;

  static std::shared_ptr<WinConfig> load( std::filesystem::path path );
  void serialize( std::filesystem::path path );

private:
  void load( sol::state const& lua );
};

#pragma once

struct WinConfig
{
  struct
  {
    int x;
    int y;
    int width;
    int height;
  } mainWindow;

  bool singleInstance;

  static WinConfig load( std::filesystem::path path );
  void serialize( std::filesystem::path path );
};

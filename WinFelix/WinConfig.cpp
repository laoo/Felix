#include "pch.hpp"
#include "WinConfig.hpp"

WinConfig WinConfig::load( std::filesystem::path path )
{
  sol::state lua;
  WinConfig result{};

  path /= "WinConfig.lua";

  if ( std::filesystem::exists( path ) )
    lua.script_file( path.string() );

  result.mainWindow.x = lua["mainWindow"]["x"].get_or( CW_USEDEFAULT );
  result.mainWindow.y = lua["mainWindow"]["y"].get_or( CW_USEDEFAULT );
  result.mainWindow.width = lua["mainWindow"]["width"].get_or( 960 );
  result.mainWindow.height = lua["mainWindow"]["height"].get_or( 630 );

  return result;
}

void WinConfig::serialize( std::filesystem::path path )
{
  path /= "WinConfig.lua";

  std::ofstream fout{ path };

  fout << "mainWindow = {\n";
  fout << "\tx = " << mainWindow.x << ";\n";
  fout << "\ty = " << mainWindow.y << ";\n";
  fout << "\twidth = " << mainWindow.width << ";\n";
  fout << "\theight = " << mainWindow.height << ";\n";
  fout << "};";
}

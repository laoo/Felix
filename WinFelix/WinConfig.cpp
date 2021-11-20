#include "pch.hpp"
#include "WinConfig.hpp"

std::shared_ptr<WinConfig> WinConfig::load( std::filesystem::path path )
{
  sol::state lua;
  auto result{ std::make_shared<WinConfig>() };

  path /= "WinConfig.lua";

  try
  {
    if ( std::filesystem::exists( path ) )
      lua.script_file( path.string() );

    result->load( lua );
  }
  catch( ... )
  {
  }

  return result;
}

void WinConfig::load( sol::state const& lua )
{
  mainWindow.x = lua["mainWindow"]["x"].get_or( CW_USEDEFAULT );
  mainWindow.y = lua["mainWindow"]["y"].get_or( CW_USEDEFAULT );
  mainWindow.width = lua["mainWindow"]["width"].get_or( 960 );
  mainWindow.height = lua["mainWindow"]["height"].get_or( 630 );
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
  fout << "};\n";
}

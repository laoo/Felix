#include "pch.hpp"
#include "SysConfig.hpp"

std::shared_ptr<SysConfig> SysConfig::load( std::filesystem::path path )
{
  sol::state lua;
  auto result{ std::make_shared<SysConfig>() };

  path /= "SysConfig.lua";

  try
  {
    if ( std::filesystem::exists( path ) )
      lua.script_file( path.string() );

    result->load( lua );
  }
  catch ( ... )
  {
  }

  return result;
}

void SysConfig::serialize( std::filesystem::path path )
{
  path /= "SysConfig.lua";

  std::ofstream fout{ path };

  fout << "mainWindow = {\n";
  fout << "\tx = " << mainWindow.x << ";\n";
  fout << "\ty = " << mainWindow.y << ";\n";
  fout << "\twidth = " << mainWindow.width << ";\n";
  fout << "\theight = " << mainWindow.height << ";\n";
  fout << "};\n";
  fout << "singleInstance = " << ( singleInstance ? "true;\n" : "false;\n" );
  fout << "bootROM = {\n";
  fout << "\tuseExternal = " << ( bootROM.useExternal ? "true;\n" : "false;\n" );
  fout << "\tpath = " << bootROM.path << ";\n";
  fout << "};\n";
  fout << "keyMapping = {\n";
  fout << "\tpause   = " << keyMapping.pause << ";\n";
  fout << "\tdown    = " << keyMapping.down << ";\n";
  fout << "\tup      = " << keyMapping.up << ";\n";
  fout << "\tright   = " << keyMapping.right << ";\n";
  fout << "\tleft    = " << keyMapping.left << ";\n";
  fout << "\toption1 = " << keyMapping.option1 << ";\n";
  fout << "\toption2 = " << keyMapping.option2 << ";\n";
  fout << "\tinner   = " << keyMapping.inner << ";\n";
  fout << "\touter   = " << keyMapping.outer << ";\n";
  fout << "};\n";
  fout << "lastOpenDirectory = " << lastOpenDirectory << ";\n";
  fout << "debugMode = " << ( debugMode ? "true;\n" : "false;\n" );
  fout << "visualizeCPU = " << ( visualizeCPU ? "true;\n" : "false;\n" );
  fout << "visualizeDisasm = " << ( visualizeDisasm ? "true;\n" : "false;\n" );
  fout << "visualizeMemory = " << ( visualizeMemory ? "true;\n" : "false;\n");
  fout << "visualizeHistory = " << ( visualizeHistory ? "true;\n" : "false;\n" );
  fout << "debugModeOnBreak = " << ( debugModeOnBreak ? "true;\n" : "false;\n" );
  fout << "normalModeOnRun = " << ( normalModeOnRun ? "true;\n" : "false;\n" );
  fout << "breakOnBrk = " << ( breakOnBrk ? "true;\n" : "false;\n" );
  fout << "screenViews = {\n";
  for ( auto const& sv : screenViews )
  {
    fout << "\t{" << sv.id << ", " << sv.type << ", " << sv.customAddress << ", " << ( sv.safePalette ? "1" : "false" ) << "};\n";
  }
  fout << "};\n";
  fout << "audio = {\n";
  fout << "\tmute = " << ( audio.mute ? "true;\n" : "false;\n" );
  fout << "};\n";
}

void SysConfig::load( sol::state const& lua )
{
  mainWindow.x = lua["mainWindow"]["x"].get_or( mainWindow.x );
  mainWindow.y = lua["mainWindow"]["y"].get_or( mainWindow.y );
  mainWindow.width = lua["mainWindow"]["width"].get_or( mainWindow.width );
  mainWindow.height = lua["mainWindow"]["height"].get_or( mainWindow.height );
  singleInstance = lua["singleInstance"].get_or( singleInstance );
  bootROM.useExternal = lua["bootROM"]["useExternal"].get_or( bootROM.useExternal );
  bootROM.path = lua["bootROM"]["path"].get_or<std::string>( {} );
  keyMapping.pause = lua["keyMapping"]["pause"].get_or( keyMapping.pause );
  keyMapping.down = lua["keyMapping"]["down"].get_or( keyMapping.down );
  keyMapping.up = lua["keyMapping"]["up"].get_or( keyMapping.up );
  keyMapping.right = lua["keyMapping"]["right"].get_or( keyMapping.right );
  keyMapping.left = lua["keyMapping"]["left"].get_or( keyMapping.left );
  keyMapping.option1 = lua["keyMapping"]["option1"].get_or( keyMapping.option1 );
  keyMapping.option2 = lua["keyMapping"]["option2"].get_or( keyMapping.option2 );
  keyMapping.inner = lua["keyMapping"]["inner"].get_or( keyMapping.inner );
  keyMapping.outer = lua["keyMapping"]["outer"].get_or( keyMapping.outer );
  lastOpenDirectory = lua["lastOpenDirectory"].get_or<std::string>( {} );
  debugMode = lua["debugMode"].get_or( debugMode );
  visualizeCPU = lua["visualizeCPU"].get_or( visualizeCPU );
  visualizeMemory = lua["visualizeMemory"].get_or(visualizeMemory);
  visualizeDisasm = lua["visualizeDisasm"].get_or( visualizeDisasm );
  visualizeHistory = lua["visualizeHistory"].get_or( visualizeHistory );
  debugModeOnBreak = lua["debugModeOnBreak"].get_or( debugModeOnBreak );
  normalModeOnRun = lua["normalModeOnRun"].get_or( normalModeOnRun );
  breakOnBrk = lua["breakOnBrk"].get_or( breakOnBrk );
  if ( auto optSV = lua.get<sol::optional<sol::table>>( "screenViews" ) )
  {
    for ( auto sv : *optSV )
    {
      auto tab = sv.second.as<sol::table>();
      screenViews.emplace_back( tab.get<int>( 1 ), tab.get<int>( 2 ), tab.get<int>( 3 ), tab.get<sol::optional<bool>>( 4 ).value_or( false ) );
    }
  }
  audio.mute = lua["audio"]["mute"].get_or( audio.mute );
}

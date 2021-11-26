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

  fout << "singleInstance = " << ( singleInstance ? "true;\n" : "false;\n" );
  fout << "kernel = {\n";
  fout << "\tuseExternal = " << ( kernel.useExternal ? "true;\n" : "false;\n" );
  fout << "\tpath = " << kernel.path << ";\n";
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
}

void SysConfig::load( sol::state const& lua )
{
  if ( sol::optional<bool> opt = lua["singleInstance"] )
    singleInstance = *opt;
  if ( sol::optional<bool> opt = lua["kernel"]["useExternal"] )
    kernel.useExternal = *opt;
  if ( sol::optional<std::string> opt = lua["kernel"]["path"] )
    kernel.path = *opt;
  if ( sol::optional<int> opt = lua["keyMapping"]["pause"] )
    keyMapping.pause = *opt;
  if ( sol::optional<int> opt = lua["keyMapping"]["down"] )
    keyMapping.down = *opt;
  if ( sol::optional<int> opt = lua["keyMapping"]["up"] )
    keyMapping.up = *opt;
  if ( sol::optional<int> opt = lua["keyMapping"]["right"] )
    keyMapping.right = *opt;
  if ( sol::optional<int> opt = lua["keyMapping"]["left"] )
    keyMapping.left = *opt;
  if ( sol::optional<int> opt = lua["keyMapping"]["option1"] )
    keyMapping.option1 = *opt;
  if ( sol::optional<int> opt = lua["keyMapping"]["option2"] )
    keyMapping.option2 = *opt;
  if ( sol::optional<int> opt = lua["keyMapping"]["inner"] )
    keyMapping.inner = *opt;
  if ( sol::optional<int> opt = lua["keyMapping"]["outer"] )
    keyMapping.outer = *opt;
}

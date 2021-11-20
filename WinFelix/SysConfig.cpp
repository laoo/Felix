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
}

void SysConfig::load( sol::state const& lua )
{
  singleInstance = lua["singleInstance"].get_or( false );
  kernel.useExternal = lua["kernel.useExternal"].get_or( false );
  kernel.path = lua["kernel.path"].get_or( std::string{} );
}

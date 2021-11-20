#pragma once

struct SysConfig
{
  bool singleInstance = false;
  struct Kernel
  {
    bool useExternal = false;
    std::filesystem::path path{};
  } kernel;

  static std::shared_ptr<SysConfig> load( std::filesystem::path path );
  void serialize( std::filesystem::path path );

private:
  void load( sol::state const& lua );
};

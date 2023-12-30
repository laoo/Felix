#include "ConfigProvider.hpp"
#include "SysConfig.hpp"

ConfigProvider::ConfigProvider() : mAppDataFolder{ obtainAppDataFolder() }, mSysConfig{}
{
  std::filesystem::create_directories( mAppDataFolder );
  mSysConfig = SysConfig::load( mAppDataFolder );
}

ConfigProvider::~ConfigProvider()
{
  serialize();
}

std::shared_ptr<SysConfig> ConfigProvider::sysConfig() const
{
  return mSysConfig;
}

std::filesystem::path ConfigProvider::appDataFolder() const
{
  return mAppDataFolder;
}

void ConfigProvider::serialize()
{
  mSysConfig->serialize( mAppDataFolder );
}

std::filesystem::path ConfigProvider::obtainAppDataFolder()
{
  wchar_t* path_tmp;
  auto ret = SHGetKnownFolderPath( FOLDERID_LocalAppData, 0, nullptr, &path_tmp );

  if ( ret == S_OK )
  {
    std::filesystem::path result = path_tmp;
    CoTaskMemFree( path_tmp );
    return result / APP_NAME;
  }
  else
  {
    CoTaskMemFree( path_tmp );
    return {};
  }
}

ConfigProvider gConfigProvider;

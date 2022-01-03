#pragma once

struct WinConfig;
struct SysConfig;

class ConfigProvider
{
public:
  ConfigProvider();
  ~ConfigProvider();

  std::shared_ptr<SysConfig> sysConfig() const;
  std::filesystem::path appDataFolder() const;

  void serialize();

private:

  static std::filesystem::path obtainAppDataFolder();
  std::shared_ptr<SysConfig> mSysConfig;

  std::filesystem::path mAppDataFolder;
};


extern ConfigProvider gConfigProvider;

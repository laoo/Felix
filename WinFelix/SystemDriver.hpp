#pragma once
#include "ISystemDriver.hpp"

class IBaseRenderer;
class IExtendedRenderer;
class UserInput;

class SystemDriver : public ISystemDriver
{
public:
  SystemDriver( HWND hWnd, std::filesystem::path const& iniPath );
  ~SystemDriver() override = default;

  std::shared_ptr<IBaseRenderer> baseRenderer() const override;
  std::shared_ptr<IExtendedRenderer> extendedRenderer() const override;
  int wndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) override;
  void quit() override;
  void update() override;
  std::shared_ptr<IUserInput> userInput() const override;
  void updateRotation( ImageProperties::Rotation rotation ) override;

  void registerDropFiles( std::function<void( std::filesystem::path )> ) override;

private:
  void handleFileDrop( HDROP hDrop );


private:
  HWND mhWnd;
  std::shared_ptr<IBaseRenderer> mBaseRenderer;
  std::shared_ptr<IExtendedRenderer> mExtendedRenderer;
  std::shared_ptr<UserInput> mIntputSource;

  std::function<void( std::filesystem::path )> mDropFilesHandler;

};
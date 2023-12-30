#pragma once
#include "ISystemDriver.hpp"

class IRenderer;
class UserInput;

class SystemDriver : public ISystemDriver
{
public:
  SystemDriver();
  void initialize( HWND hWnd );

  ~SystemDriver() override = default;

  int eventLoop() override;

  std::shared_ptr<IRenderer> renderer() const override;
  int wndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) override;
  void quit() override;
  void update() override;
  std::shared_ptr<IUserInput> userInput() const override;
  void updateRotation( ImageProperties::Rotation rotation ) override;
  void setImageName( std::wstring name ) override;
  void setPaused( bool paused ) override;

  void registerDropFiles( std::function<void( std::filesystem::path )> dropFilesHandler ) override;
  void registerUpdate( std::function<void()> updataHandler ) override;

private:
  void handleFileDrop( HDROP hDrop );


private:

  friend std::shared_ptr<ISystemDriver> createSystemDriver( Manager& manager, std::wstring const& arg, int nCmdShow );

  HWND mhWnd;
  std::shared_ptr<IRenderer> mRenderer;
  std::shared_ptr<UserInput> mIntputSource;

  std::function<void( std::filesystem::path )> mDropFilesHandler;
  std::function<void()> mUpdateHandler;

  std::wstring mImageFileName{};
  bool mPaused{};
  bool mNewPaused{};
};

#pragma once
#include "ImageProperties.hpp"

class IUserInput;
class IExtendedRenderer;
class IBaseRenderer;

class ISystemDriver
{
public:
  virtual ~ISystemDriver() = default;

  static std::shared_ptr<ISystemDriver> create( HWND hWnd, std::filesystem::path const& iniPath );

  virtual std::shared_ptr<IBaseRenderer> baseRenderer() const = 0;
  virtual std::shared_ptr<IExtendedRenderer> extendedRenderer() const = 0;
  virtual int wndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) = 0;
  virtual void quit() = 0;
  virtual void update() = 0;
  virtual std::shared_ptr<IUserInput> userInput() const = 0;
  virtual void updateRotation( ImageProperties::Rotation rotation ) = 0;

  virtual void registerDropFiles( std::function<void( std::filesystem::path )> ) = 0;

};


#pragma once
#include "BaseRenderer.hpp"

class IBaseRenderer;
class IExtendedRenderer;

class SystemDriver : public ISystemDriver
{
public:
  SystemDriver( HWND hWnd, std::filesystem::path const& iniPath );
  ~SystemDriver() override = default;

  std::shared_ptr<IBaseRenderer> baseRenderer() const override;
  std::shared_ptr<IExtendedRenderer> extendedRenderer() const override;
  int wndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) override;
  void quit() override;


private:
  HWND mhWnd;
  std::shared_ptr<IBaseRenderer> mBaseRenderer;
  std::shared_ptr<IExtendedRenderer> mExtendedRenderer;
};
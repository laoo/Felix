#include "pch.hpp"
#include "BaseRenderer.hpp"
#include "SystemDriver.hpp"

std::shared_ptr<ISystemDriver> ISystemDriver::create( HWND hWnd, std::filesystem::path const& iniPath )
{
  return std::make_shared<SystemDriver>( hWnd, iniPath );
}

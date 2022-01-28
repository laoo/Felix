#include "pch.hpp"
#include "BaseRenderer.hpp"
#include "DX9Renderer.hpp"
#include "DX11Renderer.hpp"


std::shared_ptr<IBaseRenderer> createRenderer( HWND hWnd, std::filesystem::path const& iniPath )
{
  try
  {
    return std::make_shared<DX11Renderer>( hWnd, iniPath );
  }
  catch ( ... )
  {
    return std::make_shared<DX9Renderer>( hWnd, iniPath );
  }
}

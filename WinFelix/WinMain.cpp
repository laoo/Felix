#include "pch.hpp"
#include "Core.hpp"
#include "IInputSource.hpp"
#include "Log.hpp"
#include "WinAudioOut.hpp"
#include "InputFile.hpp"
#include "Manager.hpp"
#include "ComLynxWire.hpp"
#include "ConfigProvider.hpp"
#include "SysConfig.hpp"
#include "ISystemDriver.hpp"



std::wstring getCommandArg()
{
  LPWSTR* szArgList;
  int argCount;

  szArgList = CommandLineToArgvW( GetCommandLine(), &argCount );

  if ( szArgList != NULL && argCount > 1 )
  {
    std::wstring result{ szArgList[1] };
    LocalFree( szArgList );
    return result;
  }

  return {};
}



int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
  Manager manager;
  std::shared_ptr<ISystemDriver> systemDriver;
  try
  {

    L_SET_LOGLEVEL( Log::LL_DEBUG );

    std::wstring arg = getCommandArg();

    systemDriver = createSystemDriver( manager, arg, nCmdShow );
    systemDriver->eventLoop();
    return 0;
  }
  catch ( sol::error const& err )
  {
    L_ERROR << err.what();
    MessageBoxA( nullptr, err.what(), "Error", 0 );
    return -1;
  }
  catch ( std::exception const& ex )
  {
    L_ERROR << ex.what();
    MessageBoxA( nullptr, ex.what(), "Error", 0 );
    return -1;
  }
}

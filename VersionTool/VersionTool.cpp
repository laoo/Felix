
#include <windows.h>
#include <atlstr.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <iostream>
#include <codecvt>

//https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po/35658917#35658917
//
// Execute a command and get the results. (Only standard output)
//
CStringA ExecCmd( const wchar_t* cmd              // [in] command to execute
)
{
  CStringA strResult;
  HANDLE hPipeRead, hPipeWrite;

  SECURITY_ATTRIBUTES saAttr ={ sizeof( SECURITY_ATTRIBUTES ) };
  saAttr.bInheritHandle = TRUE; // Pipe handles are inherited by child process.
  saAttr.lpSecurityDescriptor = NULL;

  // Create a pipe to get results from child's stdout.
  if ( !CreatePipe( &hPipeRead, &hPipeWrite, &saAttr, 0 ) )
    return strResult;

  STARTUPINFOW si ={ sizeof( STARTUPINFOW ) };
  si.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
  si.hStdOutput  = hPipeWrite;
  si.hStdError   = hPipeWrite;
  si.wShowWindow = SW_HIDE; // Prevents cmd window from flashing.
                            // Requires STARTF_USESHOWWINDOW in dwFlags.

  PROCESS_INFORMATION pi ={ 0 };

  BOOL fSuccess = CreateProcessW( NULL, (LPWSTR)cmd, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi );
  if ( !fSuccess )
  {
    CloseHandle( hPipeWrite );
    CloseHandle( hPipeRead );
    return strResult;
  }

  bool bProcessEnded = false;
  for ( ; !bProcessEnded;)
  {
    // Give some timeslice (50 ms), so we won't waste 100% CPU.
    bProcessEnded = WaitForSingleObject( pi.hProcess, 50 ) == WAIT_OBJECT_0;

    // Even if process exited - we continue reading, if
    // there is some data available over pipe.
    for ( ;;)
    {
      char buf[1024];
      DWORD dwRead = 0;
      DWORD dwAvail = 0;

      if ( !::PeekNamedPipe( hPipeRead, NULL, 0, NULL, &dwAvail, NULL ) )
        break;

      if ( !dwAvail ) // No data available, return
        break;

      if ( !::ReadFile( hPipeRead, buf, min( sizeof( buf ) - 1, dwAvail ), &dwRead, NULL ) || !dwRead )
        // Error, the child process might ended
        break;

      buf[dwRead] = 0;
      strResult += buf;
    }
  } //for

  CloseHandle( hPipeWrite );
  CloseHandle( hPipeRead );
  CloseHandle( pi.hProcess );
  CloseHandle( pi.hThread );
  return strResult;
} //ExecCmd

int main( int argc, char** argv )
{
  if ( argc < 2 )
    return -1;

  std::filesystem::path path{ argv[1] };

  std::string gitPath = "git.exe";
  if ( argc == 3 )
    gitPath = argv[2];


  std::string oldString{}, newString{};

  if ( std::filesystem::exists( path ) )
  {
    std::ifstream fin{ path, std::ios::binary };
    oldString.resize( (size_t)std::filesystem::file_size( path ) );
    fin.read( oldString.data(), oldString.size() );
  }


  std::string line;
  try
  {
    std::string cmd = gitPath + " describe --long";

    int wchars_num = MultiByteToWideChar( CP_UTF8, 0, cmd.c_str(), -1, NULL, 0 );
    std::vector<wchar_t> wstr( wchars_num, 0 );
    MultiByteToWideChar( CP_UTF8, 0, cmd.c_str(), -1, wstr.data(), wchars_num );
    line = ExecCmd( wstr.data() ).GetString();
  }
  catch ( std::exception const& )
  {
    return -1;
  }

  std::regex rx{ R"reg(^v(\d+)\.(\d+)(?:-(\d+)-g(.*))?)reg" };
  std::smatch sm;
  if ( std::regex_search( line, sm, rx ) )
  {
    std::stringstream ss;
    ss << "#pragma once" << std::endl;
    ss << "static constexpr auto version_major = " << sm.str( 1 ) << ";" << std::endl;
    ss << "static constexpr auto version_minor = " << sm.str( 2 ) << ";" << std::endl;
    ss << "static constexpr auto version_patch = " << sm.str( 3 ) << ";" << std::endl;
    ss << "static constexpr auto version_string = L\"" << sm.str( 1 ) << "." << sm.str( 2 ) << "." << sm.str( 3 ) << " (" << sm.str( 4 ) << ")\"" << ";" << std::endl;
    newString = ss.str();
  }

  {
    std::stringstream ss;
    ss << "Compiling Felix " << sm.str( 1 ) << "." << sm.str( 2 ) << "." << sm.str( 3 ) << " (" << sm.str( 4 ) << ")" << std::endl;
    std::cout << ss.str();
  }

  if ( newString != oldString )
  {
    std::ofstream fout{ path, std::ios::binary };
    fout.write( newString.data(), newString.size() );
  }

  return 0;
}

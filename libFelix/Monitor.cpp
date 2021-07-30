#include "pch.hpp"
#include "Monitor.hpp"
#include "Log.hpp"

Monitor::Monitor( std::filesystem::path const& sourceFolder )
{

  for ( auto& p : std::filesystem::directory_iterator( sourceFolder ) )
  {
    if ( p.path().extension() == ".lst" )
    {
      parseLst( p.path() );
    }
  }
}

Monitor::~Monitor()
{
}

void Monitor::parseLst( std::filesystem::path const& lstPath )
{
  std::ifstream fin{ lstPath, std::ios::binary };
  std::string line;
  mWatchLabel.clear();
  while ( std::getline( fin, line ) && !line.empty() )
  {
    parseLine( line );
  }
}

void Monitor::parseLine( std::string const& line )
{
  uint16_t address{};
  int lineNumber{};

  struct
  {
    std::string::const_iterator begin;
    std::string::const_iterator end;
    char const* operator()()
    {
      if ( begin < end )
        return &*begin++;
      else
        return nullptr;
    }
  } get{ line.cbegin(), line.cend() };

  for ( ;; )
  {
    if ( auto c = get() )
    {
      if ( isspace( *c ) )
        continue;
      else if ( isdigit( *c ) )
      {
        lineNumber = *c - '0';
        break;
      }
      else
        return;
    }
    else
      return;
  }

  for ( ;; )
  {
    if ( auto c = get() )
    {
      if ( isdigit( *c ) )
      {
        lineNumber = 10 * lineNumber + *c - '0';
        continue;
      }
      else if ( isspace( *c ) )
      {
        break;
      }
      else
      {
        return;
      }
    }
    else
      return;
  }

  for ( auto c = get.begin; c != get.end; ++c )
  {
    if ( isspace( *c ) )
      continue;
    else if ( *c == '*' )
    {
      get.begin = c + 1;
      std::string cmd;

      for ( ;; )
      {
        if ( auto c = get() )
        {
          if ( isspace( *c ) )
            break;
          cmd += *c;
        }
      }

      if ( cmd == "simple_watch" )
      {
        mWatchLabel = &*get.begin;
        mWatchLabel = boost::algorithm::trim_copy( mWatchLabel );
      }
      else
        L_ERROR << "Unknown monitor command " << cmd;
      break;
    }
    else
    {
      break;
    }
  }

  for ( auto c = get.begin; c != get.end; ++c )
  {
    if ( isspace( *c ) )
      continue;
    else if ( *c == '=' )
    {
      get.begin = c + 2;
      break;
    }
    else
    {
      break;
    }
  }

  for ( ;; )
  {
    for ( int i = 0; i < 4; ++i )
    {
      if ( auto c = get() )
      {
        if ( isxdigit( *c ) )
        {
          if ( isdigit( *c ) )
            address = 16 * address + *c - '0';
          else
            address = 16 * address + 10 + *c - ( isupper( *c ) ? 'A' : 'a' );
        }
        else
          return;
      }
      else
        return;
    }

    if ( address == 0xffff )
    {
      get(); //>
      get(); //space
      continue;
    }

    if ( auto c = get() )
    {
      if ( isspace( *c ) )
      {
        break;
      }
      else if ( *c == '-' )
      {
        get();  //adr
        get();  //adr
        get();  //adr
        get();  //adr
        get();  //>
        get();  //space
        break;
      }
      else
        return;
    }
  }
}


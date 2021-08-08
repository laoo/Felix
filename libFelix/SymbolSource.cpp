#include "pch.hpp"
#include "SymbolSource.hpp"
#include "Ex.hpp"

SymbolSource::SymbolSource( std::filesystem::path const& labPath ) : mSymbols{}
{
  std::ifstream fin{ labPath };

  std::string line;
  //skip two lines
  std::getline( fin, line );
  std::getline( fin, line );

  while ( std::getline( fin, line ) && !line.empty() )
  {
    if ( auto symbol = parseLine( line ) )
    {
      mSymbols.push_back( std::move( symbol ) );
    }
  }
}

SymbolSource::~SymbolSource()
{
}

std::optional<uint16_t> SymbolSource::symbol( std::string const& name ) const
{
  auto upper = boost::algorithm::to_upper_copy( name );
  auto it = std::find_if( mSymbols.cbegin(), mSymbols.cend(), [&]( Symbol const& s )
  {
    return s.name == upper;
  } );

  return it != mSymbols.cend() ? it->value : std::optional<uint16_t>{};
}

uint16_t SymbolSource::reqSymbol( std::string const& name ) const
{
  if ( auto opt = symbol( name ) )
  {
    return *opt;
  }
  else throw Ex{} << "Symbol " << name << " required";
}

SymbolSource::Symbol SymbolSource::parseLine( std::string const& line )
{
  std::istringstream is{ line };
  std::string name;
  int bank, adr;

  is >> std::hex >> bank >> adr >> name;

  if ( bank == 0 )
    return { std::move( name ), (uint16_t)adr };
  else
    return {};
}

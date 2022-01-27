#include "pch.hpp"
#include "SymbolSource.hpp"

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
  auto it = std::ranges::find( mSymbols, upper, &Symbol::name );

  return it != mSymbols.cend() ? it->value : std::optional<uint16_t>{};
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

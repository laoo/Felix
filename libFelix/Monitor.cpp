#include "pch.hpp"
#include "Monitor.hpp"
#include "Log.hpp"
#include "SymbolSource.hpp"
#include "Core.hpp"

Monitor::Monitor( std::vector<Entry> entries ) : mEntries{ std::move( entries ) }
{
}

Monitor::~Monitor()
{
}

void Monitor::populateSymbols( SymbolSource const& symbols )
{
  for ( auto & e : mEntries )
  {
    if ( auto optSymbol = symbols.symbol( e.name ) )
    {
      e.address = *optSymbol;
    }
  }
}

cppcoro::generator<std::string_view> Monitor::sample( Core const& core )
{
  char buf[128];

  uint64_t value;

  for ( auto const& e : mEntries )
  {
    switch ( e.size )
    {
    case 1:
      value = core.sampleRam( e.address );
      if ( e.hex )
      {
        co_yield std::string_view{ buf, (size_t)sprintf_s( buf, "%s: $%02llx", e.name.c_str(), value ) };
      }
      else
      {
        co_yield std::string_view{ buf, (size_t)sprintf_s( buf, "%s: %llu", e.name.c_str(), value ) };
      }
      break;
    case 2:
      value = core.sampleRam( e.address );
      value |= (int)core.sampleRam( e.address + 1 ) << 8;
      if ( e.hex )
      {
        co_yield std::string_view{ buf, (size_t)sprintf_s( buf, "%s: $%04llx", e.name.c_str(), value ) };
      }
      else
      {
        co_yield std::string_view{ buf, (size_t)sprintf_s( buf, "%s: %llu", e.name.c_str(), value ) };
      }
      break;
    case 3:
      value = core.sampleRam( e.address );
      value |= (int)core.sampleRam( e.address + 1 ) << 8;
      value |= (int)core.sampleRam( e.address + 2 ) << 16;
      if ( e.hex )
      {
        co_yield std::string_view{ buf, (size_t)sprintf_s( buf, "%s: $%06llx", e.name.c_str(), value ) };
      }
      else
      {
        co_yield std::string_view{ buf, (size_t)sprintf_s( buf, "%s: %llu", e.name.c_str(), value ) };
      }
      break;
    case 4:
      value = core.sampleRam( e.address );
      value |= (int)core.sampleRam( e.address + 1 ) << 8;
      value |= (int)core.sampleRam( e.address + 2 ) << 16;
      value |= (int)core.sampleRam( e.address + 3 ) << 24;
      if ( e.hex )
      {
        co_yield std::string_view{ buf, (size_t)sprintf_s( buf, "%s: $%08llx", e.name.c_str(), value ) };
      }
      else
      {
        co_yield std::string_view{ buf, (size_t)sprintf_s( buf, "%s: %llu", e.name.c_str(), value ) };
      }
      break;
    default:
      co_yield std::string_view{ buf, (size_t)sprintf_s( buf, "%s: bad size %d", e.name.c_str(), e.size ) };
      break;
    }
  }

}


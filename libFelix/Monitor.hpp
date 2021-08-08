#pragma once

#include "generator.hpp"

class SymbolSource;
class Core;

class Monitor
{
public:

  struct Entry
  {
    Entry( bool hex ) : name{}, address{}, size{ 1 }, hex{ hex } {}
    std::string name;
    uint16_t address;
    uint16_t size;
    bool hex;
  };

  Monitor( std::vector<Entry> entries );
  ~Monitor();

  void populateSymbols( SymbolSource const& symbols );

  cppcoro::generator<std::string_view> sample( Core const& core );


private:

  std::vector<Entry> mEntries;
};

#pragma once

#include "generator.hpp"

class SymbolSource;
class Core;

class Monitor
{
public:

  struct Entry
  {
    enum struct Type
    {
      UNKNOWN,
      HEX,
      UNSIGNED,
      SIGNED
    } type = Type::UNKNOWN;
    std::string name = {};
    uint16_t address = {};
    uint16_t size = {};
  };

  cppcoro::generator<std::string_view> sample( Core const& core );

  void addEntry( Entry entry );

private:

  std::vector<Entry> mEntries;
};

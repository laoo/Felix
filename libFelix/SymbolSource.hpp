#pragma once

class SymbolSource
{
  struct Symbol
  {
    std::string name;
    uint16_t value;

    explicit operator bool() const
    {
      return !name.empty();
    }
  };

public:
  SymbolSource();
  SymbolSource( std::filesystem::path const& labPath );
  ~SymbolSource();
  std::optional<uint16_t> symbol( std::string const& name ) const;

private:
  Symbol parseLine( std::string const& line );

private:
  std::vector<Symbol> mSymbols;
};

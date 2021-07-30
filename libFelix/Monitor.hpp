#pragma once

//experimental monitor

class Monitor
{
public:
  Monitor( std::filesystem::path const& sourceFolder );
  ~Monitor();

private:

  void parseLst( std::filesystem::path const& lstPath );
  void parseLine( std::string const& line );

private:
  std::string mWatchLabel;
};

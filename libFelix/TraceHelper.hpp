#pragma once

class TraceHelper
{
public:
  TraceHelper();
  ~TraceHelper();
  char const * addressLabel( uint16_t address ) const;

  void setTraceComment( char const* comment );
  char const* getTraceComment();

private:
  char const * map( uint16_t address, char * dest ) const;

private:
  std::array<uint32_t, 65536> mLabels;
  std::vector<char> mData;
  char const* mTraceComment;
};


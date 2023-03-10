#pragma once

#include "fmt/format.h"

//https://stackoverflow.com/questions/68675303/how-to-create-a-function-that-forwards-its-arguments-to-fmtformat-keeping-the
template <std::size_t N>
struct StaticString
{
  char str[N]{};
  constexpr StaticString( const char( &s )[N] )
  {
    std::ranges::copy( s, str );
  }
};

class TraceHelper
{
public:
  TraceHelper();
  ~TraceHelper();
  char const * addressLabel( uint16_t address ) const;
  void updateLabel( uint16_t address, const char* label );

  void enable( bool cond );

  template<StaticString fmt, typename... Args>
  void comment( Args const&... args )
  {
    if ( !mEnabled || mCommentCursor >= mTraceComment.size() )
      return;

    if ( mCommentCursor != 0 )
      mTraceComment[mCommentCursor++] = ' ';

    auto [out,size] = fmt::format_to_n( mTraceComment.begin() + mCommentCursor, mTraceComment.size() - mCommentCursor, fmt.str, args... );
    mCommentCursor = std::distance( mTraceComment.begin(), out );
  }

  std::shared_ptr<std::string_view> getTraceComment();

private:
  char const * map( uint16_t address, char * dest ) const;

private:
  std::array<uint32_t, 65536> mLabels;
  std::array<char, 1024> mTraceComment;
  std::vector<char> mData;
  std::string_view mCommentView;
  size_t mCommentCursor;
  bool mEnabled;
};


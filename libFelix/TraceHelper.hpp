#pragma once

class TraceHelper
{
public:
  TraceHelper();
  ~TraceHelper();
  char const * addressLabel( uint16_t address ) const;

  void enable( bool cond );

  template<typename FMT, typename... Args>
  void comment( FMT const& fmt, Args const&... args )
  {
    if ( !mEnabled || mCommentCursor >= mTraceComment.size() )
      return;

    if ( mCommentCursor != 0 )
      mTraceComment[mCommentCursor++] = ' ';

    auto [out,size] = std::format_to_n( mTraceComment.begin() + mCommentCursor, mTraceComment.size() - mCommentCursor, fmt, args... );
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


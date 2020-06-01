#pragma once

#include <cstdint>
#include <cassert>
#include <experimental/coroutine>

class PenUnpacker
{
public:

  enum class Status
  {
    DATA_NEEDED,
    PEN_READY,
    NEXT_LINE,
    NEXT_QUADRANT,
    NEXT_SPRITE
  };

  struct Result
  {
    Status status;
    uint8_t pixel;

    operator Status() const
    {
      return status;
    }
  };



  struct PenIterator
  {
    uint8_t* pen;
 
    friend bool operator !=( PenIterator left, PenIterator right )
    {
      return left.pen != right.pen;
    }
    //Will be co_awaited
    PenIterator * operator++() { return this; }
    uint8_t operator*() { return *pen; }
  };

  struct Line
  {
    int count;
    bool literal;
    uint8_t pen;

    //dummy. Will be co_awaited
    PenIterator begin() { return {}; }
    PenIterator end() { return {}; }
  };

  struct LineIterator
  {
    Line* line;

    friend bool operator !=( LineIterator left, LineIterator right )
    {
      return left.line != right.line;
    }
    //Will be co_awaited
    LineIterator * operator++() { return this; }
    Line & operator*() { return *line; }
  };

  struct Sprite
  {
    //dummy. Will be co_awaited
    LineIterator begin() { return {}; }
    LineIterator end() { return {}; }
  };

  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;

  struct promise_type
  {
    PenUnpacker * unpacker;
    auto get_return_object() { return PenUnpacker{ handle::from_promise( *this ) }; }
    auto initial_suspend() { return std::experimental::suspend_always{}; }
    auto final_suspend() noexcept { return std::experimental::suspend_always{}; }
    void return_void() { unpacker->setResult( { Status::NEXT_SPRITE } ); }
    void unhandled_exception() { std::terminate(); }
    auto yield_value( uint8_t pen )
    {
      unpacker->setResult( Result{ Status::PEN_READY, pen } );
      return std::experimental::suspend_always{};
    }
    auto await_transform( PenIterator it )
    {
      struct Awaiter
      {
        PenUnpacker * unpacker;
        bool await_ready() { return unpacker->nextPen(); }
        PenIterator await_resume() { return { unpacker->getPen() }; }
        void await_suspend( std::experimental::coroutine_handle<> c ) {}
      };
      return Awaiter{ unpacker };
    }
    auto await_transform( PenIterator * it )
    {
      struct Awaiter
      {
        PenIterator * it;
        PenUnpacker * unpacker;
        bool await_ready() { return unpacker->nextPen(); }
        PenIterator * await_resume() { it->pen = unpacker->getPen(); return it; }
        void await_suspend( std::experimental::coroutine_handle<> c ) {}
      };
      return Awaiter{ it, unpacker };
    }
    auto await_transform( LineIterator it )
    {
      struct Awaiter
      {
        PenUnpacker * unpacker;
        bool await_ready() { return unpacker->nextLine(); }
        LineIterator await_resume() { return { unpacker->getLine() }; }
        void await_suspend( std::experimental::coroutine_handle<> c ) {}
      };
      return Awaiter{ unpacker };
    }
    auto await_transform( LineIterator * it )
    {
      struct Awaiter
      {
        LineIterator * it;
        PenUnpacker * unpacker;
        bool await_ready() { return unpacker->nextLine(); }
        LineIterator * await_resume() { it->line = unpacker->getLine(); return it; }
        void await_suspend( std::experimental::coroutine_handle<> c ) {}
      };
      return Awaiter{ it, unpacker };
    }
  };

  PenUnpacker( handle c );
  PenUnpacker( PenUnpacker const & other ) = delete;
  PenUnpacker& operator=( PenUnpacker const & other ) = delete;
  PenUnpacker( PenUnpacker && other ) noexcept;
  PenUnpacker & operator=( PenUnpacker && other ) noexcept = delete;
  ~PenUnpacker();

  uint8_t startLine( int32_t bpp, bool totallyLiteral, uint32_t initialData );
  void feedData( uint32_t data );

  Result operator()();

private:

  void setResult( Result result )
  {
    mResult = result;
  }

  template<size_t bits>
  int pull()
  {
    uint64_t result{};
    for ( int i = 0; i < bits; ++i )
    {
      result = ( result << 1 ) | ( mShifter & 1 );
      mShifter >>= 1;
    }
    mSize -= bits;
    mTotalSize -= bits;
    assert( mSize >= 0 );
    assert( mTotalSize >= 0 );
    return ( int )result;
  }

  int pull( uint32_t bits );

  bool nextLine();
  Line * getLine();

  bool nextPen();
  uint8_t * getPen();

private:
  handle mCoro;

  uint64_t mShifter;
  int32_t mSize;       //number of bits in shifters
  int32_t mTotalSize;  //number of bits in whole line
  int mSprDoff;

  int32_t mBPP;
  bool mLiteral;

  Line mLine;
  Result mResult;
};


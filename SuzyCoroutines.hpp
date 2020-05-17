#pragma once
#include <experimental/coroutine>
#include <cassert>

struct SuzyRequest
{
  enum class Op : uint8_t
  {
    NONE = 0,
    READ,
    READ4,
    WRITE,
    WRITE4,
    MASKED_RMW,
    XOR,
    _SIZE
  };

  union
  {
    uint64_t raw;
    struct
    {
      uint32_t value;
      uint16_t address;
      Op operation;
      uint8_t mask;
    };
  };

  SuzyRequest() : raw{} {}

  void operator()()
  {
    coro();
  }

  explicit operator bool() const
  {
    return raw != 0;
  }

  std::experimental::coroutine_handle<> coro;
};

struct SuzyRead { uint16_t address; };
struct SuzyRead4 { uint16_t address; };

class SuzyCoSubroutine
{
public:

  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;

  struct promise_type
  {
    auto get_return_object() { return SuzyCoSubroutine{ handle::from_promise( *this ) }; }
    auto initial_suspend() { return std::experimental::suspend_always{}; }
    auto final_suspend() noexcept
    {
      struct Awaiter
      {
        std::experimental::coroutine_handle<> outer;
        bool await_ready() { return false; }
        void await_resume() {}
        auto await_suspend( std::experimental::coroutine_handle<> c ) { return outer; }
      };
      return Awaiter{ outer };
    }
    void return_void() {}
    void unhandled_exception() { std::terminate(); }
    auto await_transform( SuzyRequest & req )
    {
      mReq = &req;
      return std::experimental::suspend_never{};
    }
    auto await_transform( SuzyRead read )
    {
      struct Awaiter
      {
        SuzyRequest * req;
        bool await_ready() { return false; }
        uint8_t await_resume() { return ( uint8_t )req->value; }
        void await_suspend( std::experimental::coroutine_handle<> c ) { req->coro = c; }
      };
      mReq->operation = SuzyRequest::Op::READ;
      mReq->address = read.address;
      return Awaiter{ mReq };
    }
    auto await_transform( SuzyRead4 read )
    {
      struct Awaiter
      {
        SuzyRequest * req;
        bool await_ready() { return false; }
        uint32_t await_resume() { return req->value; }
        void await_suspend( std::experimental::coroutine_handle<> c ) { req->coro = c; }
      };
      mReq->operation = SuzyRequest::Op::READ4;
      mReq->address = read.address;
      return Awaiter{ mReq };
    }
    SuzyRequest * mReq;
    std::experimental::coroutine_handle<> outer;
  };

  SuzyCoSubroutine( handle c ) : coro{ c } {}
  SuzyCoSubroutine( SuzyCoSubroutine const & other ) = delete;
  SuzyCoSubroutine& operator=( SuzyCoSubroutine const & other ) = delete;
  SuzyCoSubroutine( SuzyCoSubroutine && other ) noexcept : coro{ std::move( other.coro ) }
  {
    other.coro = nullptr;
  }
  SuzyCoSubroutine & operator=( SuzyCoSubroutine && other ) noexcept
  {
    std::swap( coro, other.coro );
    return *this;
  }
  ~SuzyCoSubroutine()
  {
    if ( coro )
      coro.destroy();
  }

  void setOuter( std::experimental::coroutine_handle<> outer )
  {
    coro.promise().outer = outer;
  }

  std::experimental::coroutine_handle<> getInner() const
  {
    return coro;
  }

private:
  handle coro;
};



class PixelUnpacker
{
public:

  enum class Status
  {
    DATA_NEEDED,
    PIXEL_READY,
    END_OF_LINE,
    END_OF_QUADRANT,
    END_OF_SPRITE
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

  struct LineData {};


  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;

  struct promise_type
  {
    PixelUnpacker * unpacker;
    auto get_return_object() { return PixelUnpacker{ handle::from_promise( *this ) }; }
    auto initial_suspend() { return std::experimental::suspend_always{}; }
    auto final_suspend() noexcept { return std::experimental::suspend_always{}; }
    void return_void() {}
    void unhandled_exception() { std::terminate(); }
    auto await_transform( [[maybe_unused]] LineData ) { return std::experimental::suspend_always{}; }
  };

  PixelUnpacker( handle c );
  PixelUnpacker( PixelUnpacker const & other ) = delete;
  PixelUnpacker& operator=( PixelUnpacker const & other ) = delete;
  PixelUnpacker( PixelUnpacker && other ) noexcept;
  PixelUnpacker & operator=( PixelUnpacker && other ) noexcept;
  ~PixelUnpacker();

  uint8_t startLine( uint32_t bpp, bool totallyLiteral, uint32_t initialData );

private:
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
    return (int)result;
  }

  void populateShifter( uint32_t data );

private:
  handle mCoro;

  uint64_t mShifter;
  int32_t mSize;       //number of bits in shifters
  int32_t mTotalSize;  //number of bits in whole line

  uint32_t mBPP;
  bool mTotallyLiteral;
};




class SuzySpriteProcessor
{
public:

  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;

  struct promise_type
  {
    auto get_return_object() { return SuzySpriteProcessor{ handle::from_promise( *this ) }; }
    auto initial_suspend() { return std::experimental::suspend_never{}; }
    auto final_suspend() noexcept { return std::experimental::suspend_always{}; }
    void return_void()
    {
      mReq->raw = 0;
    }
    void unhandled_exception() { std::terminate(); }
    auto await_transform( SuzyRequest & req )
    {
      mReq = &req;
      struct Awaiter
      {
        SuzyRequest * req;
        bool await_ready() { return false; }
        void await_resume() {}
        void await_suspend( std::experimental::coroutine_handle<> c ) { req->coro = c; }
      };
      return Awaiter{ &req };
    }
    auto await_transform( SuzyCoSubroutine && suzyCoro )
    {
      struct Awaiter
      {
        SuzyCoSubroutine coro;
        Awaiter( SuzyCoSubroutine && s ) : coro{ std::move( s ) } {}
        bool await_ready() { return false; }
        void await_resume() {}
        auto await_suspend( std::experimental::coroutine_handle<> c ) { coro.setOuter( c ); return coro.getInner(); }
      };
      return Awaiter{ std::move( suzyCoro ) };
    }

    SuzyRequest * mReq;
  };

  SuzySpriteProcessor() : coro{} {}
  SuzySpriteProcessor( handle c ) : coro{ c } {}
  SuzySpriteProcessor( SuzySpriteProcessor const & other ) = delete;
  SuzySpriteProcessor & operator=( SuzySpriteProcessor const & other ) = delete;
  SuzySpriteProcessor( SuzySpriteProcessor && other ) noexcept : coro{ std::move( other.coro ) }
  {
    other.coro = nullptr;
  }
  SuzySpriteProcessor & operator=( SuzySpriteProcessor && other ) noexcept
  {
    std::swap( coro, other.coro );
    return *this;
  }
  ~SuzySpriteProcessor()
  {
    if ( coro )
      coro.destroy();
  }

private:
  handle coro;

};



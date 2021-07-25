#pragma once
#include "Utility.hpp"

class GameDrive
{
public:

  enum class ECommandByte : uint8_t
  {
    OpenDir = 0,
    ReadDir,
    OpenFile,
    GetSize,
    Seek,
    Read,
    Write,
    Close,
    ProgramFile,
    ClearBlocks,
    LowPowerMode,
    CommandsCount
  };

  enum class FRESULT : uint8_t
  {
    OK = 0,			/* 0 */
    DISK_ERR,		/* 1 */
    NOT_READY,		/* 2 */
    NO_FILE,			/* 3 */
    NOT_OPENED,		/* 4 */
    NOT_ENABLED,		/* 5 */
    NO_FILESYSTEM	/* 6 */
  };

  bool hasOutput( uint64_t tick ) const;
  bool running() const;

  void put( uint8_t value );
  uint8_t get( uint64_t tick );

  GameDrive( std::filesystem::path const& imagePath );
  ~GameDrive();

private:

  std::filesystem::path mBasePath;

  struct Buffer
  {
    bool await_ready() { return false; }
    void await_suspend( std::coroutine_handle<> c ) {}
    uint8_t value;
  } mBuffer;

  auto& getCommand()
  {
    struct GetCommand : public Buffer
    {
      uint8_t await_resume() { return value; }
    };
    mHasOutput = false;
    mRunning = false;
    return static_cast<GetCommand&>( mBuffer );
  }

  auto& getByte()
  {
    struct GetByte : public Buffer
    {
      uint8_t await_resume() { return value; }
    };
    mHasOutput = false;
    mRunning = true;
    return static_cast<GetByte&>( mBuffer );
  }

  struct GDCoroutine : private NonCopyable
  {
  public:
    struct promise_type;
    using handle = std::coroutine_handle<promise_type>;

    struct promise_type
    {
      promise_type( GameDrive& gd ) : mGD{ gd } {}
      auto get_return_object() { return GDCoroutine{ std::coroutine_handle<promise_type>::from_promise( *this ) }; }
      auto initial_suspend() { return std::suspend_never{}; }
      void return_void() {}
      void unhandled_exception() { std::terminate(); }
      auto final_suspend() noexcept { return std::suspend_always{}; }
      auto yield_value( uint8_t value )
      {
        struct Yield : public Buffer
        {
          void await_resume() {}
        };
        mGD.mHasOutput = true;
        mGD.mRunning = true;
        mGD.mBuffer.value = value;
        return static_cast<Yield&>( mGD.mBuffer );
      }

      auto yield_value( FRESULT value )
      {
        struct Yield : public Buffer
        {
          void await_resume() {}
        };
        mGD.mHasOutput = true;
        mGD.mRunning = true;
        mGD.mBuffer.value = (uint8_t)value;
        return static_cast<Yield&>( mGD.mBuffer );
      }

    private:
      GameDrive& mGD;
    };

    GDCoroutine( handle c ) : mCoro{ c } {}
    ~GDCoroutine()
    {
      if ( mCoro )
        mCoro.destroy();
    }

    void resume() const
    {
      assert( !mCoro.done() );
      mCoro();
    }

  private:
    handle mCoro;
  } const mGDCoroutine;

private:
  GDCoroutine process();
  bool mHasOutput;
  bool mRunning;
};

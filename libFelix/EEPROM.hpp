#pragma once
#include "Utility.hpp"

class ImageCart;
class TraceHelper;

class EEPROM
{
public:

  EEPROM( std::filesystem::path imagePath, int eeType, bool is16Bit, std::shared_ptr<TraceHelper> traceHelper );
  ~EEPROM();

  static std::unique_ptr<EEPROM> create( ImageCart const& cart, std::shared_ptr<TraceHelper> traceHelper );

  void tick( uint64_t tick, bool cs, bool audin );
  std::optional<bool> output( uint64_t tick ) const;

private:

  struct NoCS {};

  struct IO
  {
    bool await_ready() { return false; }
    void await_suspend( std::coroutine_handle<> c ) {}
    void await_resume()
    {
      if ( started && !cs )
        throw NoCS{};
    }
    uint64_t currentTick;
    uint64_t busyUntil;
    bool input;
    std::optional<bool> output;
    bool cs;
    bool started;
  } mIO;

  auto& start()
  {
    struct Start : public IO
    {
      bool await_resume()
      {
        if ( input && busyUntil < currentTick )
        {
          started = true;
          output = std::nullopt;
          return true;
        }
        else
        {
          return false;
        }
      }
    };
    return static_cast<Start&>( mIO );
  }

  auto& input()
  {
    struct Input : public IO
    {
      int await_resume()
      {
        if ( started && !cs )
          throw NoCS{};
        return input ? 1 : 0;
      }
    };
    return static_cast<Input&>( mIO );
  }

  auto& finish()
  {
    struct Finish : public IO
    {
      bool await_resume()
      {
        if ( cs )
        {
          return false;
        }
        else
        {
          started = false;
          output = true;
          return true;
        }
      }
    };
    return static_cast<Finish&>( mIO );
  }

  int read( int address ) const;
  void ewen();
  void erase( int address );
  void write( int address, int data, bool erase = false );
  void eral();
  void wral( int data );
  void ewds();

  struct EECoroutine : private NonCopyable
  {
  public:
    struct promise_type;
    using handle = std::coroutine_handle<promise_type>;

    struct promise_type
    {
      promise_type( EEPROM& ee ) : mEE{ ee } {}
      auto get_return_object() { return EECoroutine{ std::coroutine_handle<promise_type>::from_promise( *this ) }; }
      auto initial_suspend() { return std::suspend_never{}; }
      void return_void() {}
      void unhandled_exception() { std::terminate(); }
      auto final_suspend() noexcept { return std::suspend_always{}; }
      auto& yield_value( int value )
      {
        mEE.mIO.output = value;
        return mEE.mIO;
      }

    private:
      EEPROM& mEE;
    };

    EECoroutine( handle c ) : mCoro{ c } {}
    ~EECoroutine()
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
  } const mEECoroutine;


  private:
    EECoroutine process();
    std::shared_ptr<TraceHelper> mTraceHelper;
    std::vector<uint8_t> mData;
    int mAddressBits;
    int mDataBits;
    bool mWriteEnable;

    static constexpr uint64_t WRITE_TICKS = 10 * 16;
    static constexpr uint64_t ERAL_TICKS = 15 * 16;
    static constexpr uint64_t WRAL_TICKS = 30 * 16;
};

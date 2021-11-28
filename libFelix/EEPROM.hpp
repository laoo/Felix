#pragma once
#include "Utility.hpp"

class ImageCart;
class TraceHelper;
class ImageProperties;

class EEPROM
{
public:

  EEPROM( std::filesystem::path imagePath, int eeType, bool is16Bit, std::shared_ptr<TraceHelper> traceHelper );
  ~EEPROM();

  static std::unique_ptr<EEPROM> create( ImageProperties const& imageProperties, std::shared_ptr<TraceHelper> traceHelper );

  void tick( uint64_t tick, bool cs, bool audin );
  std::optional<bool> output( uint64_t tick ) const;

private:

  struct NoCS {};

  struct IO
  {
    bool await_ready() { return false; }
    void await_suspend( std::coroutine_handle<> c ) {}
    int await_resume()
    {
      return input ? 1 : 0;
    }
    uint64_t currentTick;
    uint64_t busyUntil;
    bool cs;
    bool input;
    std::optional<bool> output;
  } io;

  int read( int address ) const;
  void ewen();
  void erase( int address );
  void write( int address, int data, bool erase = false );
  void eral();
  void wral( int data );
  void ewds();

  void startProgram( uint64_t duration );

  struct EECoroutine : private NonCopyable
  {
  public:
    struct promise_type;
    using handle = std::coroutine_handle<promise_type>;

    struct promise_type
    {
      promise_type( EEPROM& ee ) : mEE{ ee }
      {
      }
      auto get_return_object()
      {
        return EECoroutine{ std::coroutine_handle<promise_type>::from_promise( *this ) };
      }
      auto initial_suspend()
      {
        return std::suspend_never{};
      }
      void return_value( std::optional<bool> opt );
      void unhandled_exception()
      {
        std::terminate();
      }
      auto final_suspend() noexcept
      {
        return std::suspend_always{};
      }
      auto& yield_value( int value )
      {
        mEE.io.output = value;
        return mEE.io;
      }

    private:
      EEPROM& mEE;
    };

    EECoroutine() : mCoro{} {}
    EECoroutine( handle c ) : mCoro{ c } {}
    EECoroutine & operator=( EECoroutine&& other )
    {
      std::swap( mCoro, other.mCoro );
      return *this;
    }

    ~EECoroutine()
    {
      reset();
    }

    void operator()()
    {
      if ( mCoro )
      {
        mCoro();
        if ( mCoro.done() )
        {
          mCoro.destroy();
          mCoro = nullptr;
        }
      }
    }

    explicit operator bool() const
    {
      return (bool)mCoro;
    }

    void reset()
    {
      if ( mCoro )
        mCoro.destroy();
      mCoro = nullptr;
    }

  private:
    handle mCoro;
  } mEECoroutine;


  private:
    EECoroutine process();
    std::filesystem::path mImagePath;
    std::shared_ptr<TraceHelper> mTraceHelper;
    std::vector<uint8_t> mData;
    int mOpcodeBits;  //command with address
    int mAddressMask;
    int mDataBits;
    bool mWriteEnable;
    bool mChanged;

    static constexpr uint64_t WRITE_TICKS = 10 * 16;
    static constexpr uint64_t ERAL_TICKS = 15 * 16;
    static constexpr uint64_t WRAL_TICKS = 30 * 16;
};

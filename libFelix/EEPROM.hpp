#pragma once
#include "Utility.hpp"

class ImageCart;

class EEPROM
{
public:

  EEPROM( std::filesystem::path imagePath, int eeType, bool is16Bit );
  ~EEPROM();

  static std::unique_ptr<EEPROM> create( ImageCart const& cart );

  void tick();

private:

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
};

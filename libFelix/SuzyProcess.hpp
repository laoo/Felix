#pragma once
#include "Suzy.hpp"
#include "Utility.hpp"

class SuzyProcess : public ISuzyProcess
{
public:
  struct Response
  {
    bool await_ready() { return false; }
    void await_suspend( std::coroutine_handle<> c ) {}
    uint32_t value;
  };

public:

  SuzyProcess( Suzy & suzy );
  ~SuzyProcess() override = default;
  Request const* advance() override;
  void respond( uint32_t value ) override;

private:

  void setFinish()
  {
    mSuzy.mSpriteWorking = false;
    request = { Request::FINISH };
  }

  auto & suzyRead( uint16_t address )
  {
    struct SuzyReadResponse : public Response
    {
      uint8_t await_resume() { return (uint8_t)value; }
    };
    request = { Request::READ, address };
    return static_cast<SuzyReadResponse &>( response );
  }

  auto & suzyFetchSCB( uint16_t address )
  {
    struct SuzyFetchSCBResponse : public Response
    {
      uint8_t await_resume() { return (uint8_t)value; }
    };
    request = { Request::FETCHSCB, address };
    return static_cast<SuzyFetchSCBResponse &>( response );
  }

  auto & suzyRead4( uint16_t address )
  {
    struct SuzyRead4Response : public Response
    {
      uint32_t await_resume() { return value; }
    };
    request = { Request::READ4, address };
    return static_cast<SuzyRead4Response &>( response );
  }

  auto & suzyReadPal( uint16_t address )
  {
    struct SuzyReadPalResponse : public Response
    {
      uint32_t await_resume() { return value; }
    };
    request = { Request::READPAL, address };
    return static_cast<SuzyReadPalResponse &>( response );
  }

  auto & suzyWrite( uint16_t address, uint8_t value )
  {
    struct SuzyWriteResponse : public Response
    {
      void await_resume() {}
    };
    request = { Request::WRITE,  address, value };
    return static_cast<SuzyWriteResponse &>( response );
  }

  auto & suzyWriteFred( uint16_t address, uint8_t value )
  {
    struct SuzyWriteResponse : public Response
    {
      void await_resume() {}
    };
    request = { Request::WRITEFRED,  address, value };
    return static_cast<SuzyWriteResponse &>( response );
  }

  auto & suzyColRMW( uint32_t mask, uint16_t address, uint16_t value )
  {
    struct SuzyColRMWResponse : public Response
    {
      uint32_t await_resume() { return value; }
    };
    request = { Request::COLRMW, address, value, mask };
    return static_cast<SuzyColRMWResponse &>( response );
  }

  auto & suzyVidRMW( uint16_t address, uint8_t value, uint8_t mask )
  {
    struct SuzyVidRMWResponse : public Response
    {
      void await_resume() {}
    };
    request = { Request::VIDRMW, address, value, mask };
    return static_cast<SuzyVidRMWResponse &>( response );
  }

  auto & suzyXOR( uint16_t address, uint8_t value )
  {
    struct SuzyXORResponse : public Response
    {
      void await_resume() {}
    };
    request = { Request::XOR, address, value };
    return static_cast<SuzyXORResponse &>( response );
  }

  struct ProcessCoroutine : private NonCopyable
  {
  public:
    struct promise_type;
    using handle = std::coroutine_handle<promise_type>;

    struct promise_type
    {
      promise_type( SuzyProcess & suzyProcess ) : mSuzyProcess{ suzyProcess } {}
      auto get_return_object() { return ProcessCoroutine{ std::coroutine_handle<promise_type>::from_promise( *this ) }; }
      std::suspend_always initial_suspend() { return {}; }
      void return_void() {}
      void unhandled_exception() { std::terminate(); }

      std::suspend_always final_suspend() noexcept
      {
        mSuzyProcess.setFinish();
        return {};
      }

    private:
      SuzyProcess & mSuzyProcess;
    };

    ProcessCoroutine( handle c ) : mCoro{ c } {}
    ~ProcessCoroutine()
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
  } const mProcessCoroutine;

private:
  ProcessCoroutine process();

private:
  Suzy & mSuzy;

  Request request;
  Response response;

  bool mEveron;
};

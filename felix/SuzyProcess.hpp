#pragma once
#include "Suzy.hpp"
#include "Shifter.hpp"

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

  void setFinish();

private:

  auto & suzyRead( uint16_t address )
  {
    struct SuzyReadResponse : public Response
    {
      uint8_t await_resume() { return (uint8_t)value; }
    };
    requestRead = ISuzyProcess::RequestRead{ address };
    return static_cast<SuzyReadResponse &>( response );
  }

  auto & suzyRead4( uint16_t address )
  {
    struct SuzyRead4Response : public Response
    {
      uint32_t await_resume() { return value; }
    };
    requestRead4 = ISuzyProcess::RequestRead4{ address };
    return static_cast<SuzyRead4Response &>( response );
  }

  auto & suzyWrite( uint16_t address, uint8_t value )
  {
    struct SuzyWriteResponse : public Response
    {
      void await_resume() {}
    };
    requestWrite = ISuzyProcess::RequestWrite{ address, value };
    return static_cast<SuzyWriteResponse &>( response );
  }

  auto & suzyColRMW( uint32_t mask, uint16_t address, uint8_t value )
  {
    struct SuzyColRMWResponse : public Response
    {
      uint8_t await_resume() { return (uint8_t)value; }
    };
    requestWrite4 = ISuzyProcess::RequestColRMW{ address, mask, value };
    return static_cast<SuzyColRMWResponse &>( response );
  }

  auto & suzyVidRMW( uint16_t address, uint8_t value, uint8_t mask )
  {
    struct SuzyVidRMWResponse : public Response
    {
      void await_resume() {}
    };
    requestVidRMW = ISuzyProcess::RequestVidRMW{ address, value, mask };
    return static_cast<SuzyVidRMWResponse &>( response );
  }

  auto & suzyXOR( uint16_t address, uint8_t value )
  {
    struct SuzyXORResponse : public Response
    {
      void await_resume() {}
    };
    requestXOR = ISuzyProcess::RequestXOR{ address, value };
    return static_cast<SuzyXORResponse &>( response );
  }

  struct ProcessCoroutine
  {
  public:
    struct promise_type;
    using handle = std::coroutine_handle<promise_type>;

    struct promise_type
    {
      promise_type( SuzyProcess & suzyProcess ) : mSuzyProcess{ suzyProcess }
      {
      }

      auto get_return_object() { return ProcessCoroutine{ std::coroutine_handle<promise_type>::from_promise( *this ) }; }

      auto initial_suspend() { return std::suspend_always{}; }
      std::suspend_always final_suspend() noexcept
      {
        mSuzyProcess.setFinish();
        return {};
      }
      void return_void()
      {
      }
      void unhandled_exception() { std::terminate(); }

    private:
      SuzyProcess & mSuzyProcess;
    };


    ProcessCoroutine() : mCoro{} {}
    ProcessCoroutine( handle c ) : mCoro{ c } {}
    ProcessCoroutine( ProcessCoroutine const& other ) = delete;
    ProcessCoroutine & operator=( ProcessCoroutine const& other ) = delete;
    ProcessCoroutine( ProcessCoroutine && other ) noexcept : mCoro{ std::move( other.mCoro ) }
    {
      other.mCoro = nullptr;
    }
    ProcessCoroutine & operator=( ProcessCoroutine && other ) noexcept
    {
      mCoro = std::move( other.mCoro );
      other.mCoro = nullptr;
      return *this;
    }
    ~ProcessCoroutine()
    {
      if ( mCoro )
        mCoro.destroy();
    }

    void resume()
    {
      assert( !mCoro.done() );
      mCoro();
    }

    handle coro()
    {
      return mCoro;
    }

  private:
    handle mCoro;
  };

private:
  ProcessCoroutine process();

private:
  Suzy & mSuzy;
  Suzy::SCB & mScb;

  union
  {
    Request request;
    RequestFinish requestFinish;
    RequestRead requestRead;
    RequestRead4 requestRead4;
    RequestWrite requestWrite;
    RequestColRMW requestWrite4;
    RequestVidRMW requestVidRMW;
    RequestXOR requestXOR;
  };

  Response response;

  ProcessCoroutine mProcessCoroutine;
  bool mEveron;
};

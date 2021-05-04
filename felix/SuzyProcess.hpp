#pragma once
#include "Suzy.hpp"
#include "Shifter.hpp"

class SuzyProcess : public ISuzyProcess
{
public:
  struct Response : public Request
  {
    uint16_t addr;
    uint32_t value;
  };

public:

  SuzyProcess( Suzy & suzy );
  ~SuzyProcess() override = default;
  Request const* advance() override;
  void respond( uint32_t value ) override;

  void setFinish();
  void setRead( uint16_t address );
  void setRead4( uint16_t address );
  void setWrite( uint16_t address, uint8_t value );
  void setColRMW( uint16_t address, uint32_t mask, uint8_t value );
  void setVidRMW( uint16_t address, uint8_t value, uint8_t mask );
  void setXor( uint16_t address, uint8_t value );
  Response const& getResponse() const;
  void setHandle( std::coroutine_handle<> c );

private:

  struct SuzyRead { uint16_t address; };
  struct SuzyRead4 { uint16_t address; };
  struct SuzyWrite { uint16_t address; uint8_t value; };
  struct SuzyColRMW { uint32_t mask; uint16_t address;  uint8_t value; };
  struct SuzyVidRMW { uint16_t address; uint8_t value; uint8_t mask; };
  struct SuzyXOR { uint16_t address; uint8_t value; };

  struct ProcessCoroutine
  {
  public:
    struct promise_type;
    using handle = std::coroutine_handle<promise_type>;

    struct promise_type
    {
      promise_type( SuzyProcess & suzyProcess ) : mSuzyProcess{ suzyProcess }
      {
        mSuzyProcess.setHandle( std::coroutine_handle<promise_type>::from_promise( *this ) );
      }

      auto get_return_object() { return ProcessCoroutine{ std::coroutine_handle<promise_type>::from_promise( *this ) }; }

      auto initial_suspend() { return std::suspend_never{}; }
      std::suspend_always final_suspend() noexcept
      {
        mSuzyProcess.setFinish();
        return {};
      }
      void return_void()
      {
      }
      void unhandled_exception() { std::terminate(); }

      auto await_transform( SuzyRead r )
      {
        struct Awaiter
        {
          SuzyProcess & p;
          bool await_ready() { return false; }
          uint8_t await_resume() { return p.getResponse().value; }
          void await_suspend( std::coroutine_handle<> c ) {}
        };
        mSuzyProcess.setRead( r.address );
        return Awaiter{ mSuzyProcess };
      }
      auto await_transform( SuzyRead4 r )
      {
        struct Awaiter
        {
          SuzyProcess & p;
          bool await_ready() { return false; }
          uint32_t await_resume() { return p.getResponse().value; }
          void await_suspend( std::coroutine_handle<> c ) {}
        };
        mSuzyProcess.setRead4( r.address );
        return Awaiter{ mSuzyProcess };
      }
      auto await_transform( SuzyWrite w )
      {
        struct Awaiter
        {
          SuzyProcess & p;
          bool await_ready() { return false; }
          void await_resume() {}
          void await_suspend( std::coroutine_handle<> c ) {}
        };
        mSuzyProcess.setWrite( w.address, w.value );
        return Awaiter{ mSuzyProcess };
      }
      auto await_transform( SuzyColRMW w )
      {
        struct Awaiter
        {
          SuzyProcess & p;
          bool await_ready() { return false; }
          uint8_t await_resume() { return (uint8_t)p.getResponse().value; }
          void await_suspend( std::coroutine_handle<> c ) {}
        };
        mSuzyProcess.setColRMW( w.address, w.mask, w.value );
        return Awaiter{ mSuzyProcess };
      }
      auto await_transform( SuzyVidRMW rmw )
      {
        struct Awaiter
        {
          SuzyProcess & p;
          bool await_ready() { return false; }
          void await_resume() {}
          void await_suspend( std::coroutine_handle<> c ) {}
        };
        mSuzyProcess.setVidRMW( rmw.address, rmw.value, rmw.mask );
        return Awaiter{ mSuzyProcess };
      }
      auto await_transform( SuzyXOR x )
      {
        struct Awaiter
        {
          SuzyProcess & p;
          bool await_ready() { return false; }
          void await_resume() {}
          void await_suspend( std::coroutine_handle<> c ) {}
        };
        mSuzyProcess.setXor( x.address, x.value );
        return Awaiter{ mSuzyProcess };
      }

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
    Response response;
  };

  ProcessCoroutine mBaseCoroutine;
  std::coroutine_handle<> mCoro;
  bool mEveron;
};

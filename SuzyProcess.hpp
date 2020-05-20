#pragma once
#include "Suzy.hpp"
#include "SuzyCoroutines.hpp"

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
  void setWrite4( uint16_t address, uint32_t value );
  void setRMW( uint16_t address, uint8_t value, uint8_t mask );
  void setXor( uint16_t address, uint8_t value );
  Response const& getResponse() const;
  void setHandle( std::experimental::coroutine_handle<> c );


  struct ProcessCoroutine : public BaseCoroutine {};
  struct SubCoroutine : public BaseCoroutine{};
  template<typename RET>
  struct SubCoroutineT : public BaseCoroutine {};


private:
  ProcessCoroutine process();
  SubCoroutine loadSCB();
  SubCoroutineT<bool> renderSingleSprite();

private:
  Suzy & mSuzy;
  Suzy::SCB & scb;

  union
  {
    Request request;
    RequestFinish requestFinish;
    RequestRead requestRead;
    RequestRead4 requestRead4;
    RequestWrite requestWrite;
    RequestWrite4 requestWrite4;
    RequestRMW requestRMW;
    RequestXOR requestXOR;
    Response response;
  };

  BaseCoroutine mBaseCoroutine;
  std::experimental::coroutine_handle<> mCoro;
};

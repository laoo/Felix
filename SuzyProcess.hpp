#pragma once
#include "Suzy.hpp"
#include "SuzyProcessCoro.hpp"
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
  void setWrite4( uint16_t address, uint32_t value );
  void setRMW( uint16_t address, uint8_t value, uint8_t mask );
  void setXor( uint16_t address, uint8_t value );
  Response const& getResponse() const;
  void setHandle( std::experimental::coroutine_handle<> c );
  AssemblePen & assemblePen( int pen, int count );
  AssemblePen & getPen();
  void initPen( std::experimental::coroutine_handle<> handle );

private:
  ProcessCoroutine process();
  SubCoroutine loadSCB();
  SubCoroutineT<bool> renderSingleSprite();
  PenAssemblerCoroutine penAssembler();

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

  ProcessCoroutine mBaseCoroutine;
  std::experimental::coroutine_handle<> mCoro;
  std::experimental::coroutine_handle<PenAssemblerPromise<PenAssemblerCoroutine>> mPenAssemblerHandle;
  AssemblePen mAssembledPen;
  Shifter mShifter;
  int sprhpos;
  uint16_t hsizacum;

};

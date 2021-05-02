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
  void setColRMW( uint16_t address, uint32_t mask, uint8_t value );
  void setVidRMW( uint16_t address, uint8_t value, uint8_t mask );
  void setXor( uint16_t address, uint8_t value );
  Response const& getResponse() const;
  void setHandle( std::coroutine_handle<> c );
  AssemblePen & readPen();
  AssemblePen & readHeader();
  AssemblePen & duplicatePen();
  AssemblePen & flush();
  AssemblePen & newLine();
  AssemblePen & finish();
  AssemblePen & getPen();
  void initPen( std::coroutine_handle<> handle );

private:
  ProcessCoroutine process();
  static SubCoroutine renderSingleSprite( SuzyProcess * self );

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
    RequestColRMW requestWrite4;
    RequestVidRMW requestVidRMW;
    RequestXOR requestXOR;
    Response response;
  };

  ProcessCoroutine mBaseCoroutine;
  std::coroutine_handle<> mCoro;
  AssemblePen mAssembledPen;
  Shifter mShifter;
  uint16_t sprhpos;
  uint16_t hsizacum;
  int left;
  bool mEveron;
};

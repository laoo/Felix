#pragma once

struct IVideoSink
{
  virtual ~IVideoSink() = default;

  virtual void newFrame( uint64_t tick, uint8_t hbackup ) = 0;
  //row counts from 104 to 0
  virtual void newRow( uint64_t tick, int row ) = 0;

  virtual void emitScreenData( std::span<uint8_t const> data ) = 0;
  virtual void updateColorReg( uint8_t reg, uint8_t value ) = 0;
};


#pragma once

class IVideoSink
{
public:
  virtual ~IVideoSink() = default;

  virtual void startNewFrame( uint64_t tick ) = 0;

  virtual void emitScreenData( std::span<uint8_t const> data ) = 0;
  virtual void updateColorReg( uint8_t reg, uint8_t value ) = 0;

};


#pragma once

class IVideoSink
{
public:
  virtual ~IVideoSink() = default;

  virtual void startNewFrame() = 0;

  virtual void emitScreenData( std::span<uint8_t const> data ) = 0;
  virtual void updateColorReg( uint16_t reg, uint8_t value ) = 0;
  virtual void render() = 0;  //temporary

};


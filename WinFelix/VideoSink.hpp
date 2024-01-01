#pragma once
#include "IVideoSink.hpp"

class ScreenRenderingBuffer;

struct VideoSink : public IVideoSink
{
  struct Pixel
  {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t x;
  };

  struct DPixel
  {
    Pixel left;
    Pixel right;
  };

  VideoSink();

  std::array<DPixel, 256> mPalette;
  std::shared_ptr<ScreenRenderingBuffer> mActiveFrame;
  mutable std::mutex mQueueMutex;

  void updatePalette( uint16_t reg, uint8_t value );
  void newFrame() override;
  void newRow( int row ) override;
  void emitScreenData( std::span<uint8_t const> data ) override;
  void updateColorReg( uint8_t reg, uint8_t value ) override;
  std::shared_ptr<ScreenRenderingBuffer> pullNextFrame();
};


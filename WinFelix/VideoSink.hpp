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
  std::queue<std::shared_ptr<ScreenRenderingBuffer>> mFinishedFrames;
  mutable std::mutex mQueueMutex;
  uint64_t mBeginTick;
  uint64_t mLastTick;
  uint64_t mFrameTicks;

  void updatePalette( uint16_t reg, uint8_t value );
  void newFrame( uint64_t tick, uint8_t hbackup ) override;
  void newRow( uint64_t tick, int row ) override;
  void emitScreenData( std::span<uint8_t const> data ) override;
  void updateColorReg( uint8_t reg, uint8_t value ) override;
  std::shared_ptr<ScreenRenderingBuffer> pullNextFrame();
};


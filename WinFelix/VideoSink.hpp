#pragma once
#include "IVideoSink.hpp"

struct VideoSink : public IVideoSink
{
  VideoSink();
  void newFrame() override;
  Doublet* getRow( int row ) override;

  std::array<Doublet, ROW_BYTES * SCREEN_HEIGHT> frame;
};


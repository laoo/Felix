#include "VideoSink.hpp"

VideoSink::VideoSink() : frame{}
{
  std::ranges::fill( frame, Doublet{} );
}

void VideoSink::newFrame()
{
}

Doublet* VideoSink::getRow( int row )
{
  assert( row >= 0 && row < SCREEN_HEIGHT );
  return frame.data() + row * ROW_BYTES;
}

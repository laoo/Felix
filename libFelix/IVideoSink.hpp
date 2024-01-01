#pragma once

#include "Utility.hpp"

struct IVideoSink
{
  virtual ~IVideoSink() = default;

  virtual void newFrame() = 0;
  //row counts from 0 to 101
  virtual Doublet* getRow( int row ) = 0;
};


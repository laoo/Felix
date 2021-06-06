#pragma once
#include "DisplayLine.hpp"

class IVideoSink
{
public:
  virtual ~IVideoSink() = default;

  /**
  * Asks for a next DisplayLine instance to put display data
  */
  virtual DisplayLine * getNextLine( int32_t displayRow ) = 0;

  /**
  * Used to statically update color register when there is no active DisplayLine instance 
  */
  virtual void updateColorReg( uint8_t value, uint8_t reg ) = 0;

};


#pragma once

#include "IInputSource.hpp"

class IUserInput : public IInputSource
{
public:
  virtual ~IUserInput() = default;

  virtual int getVirtualCode( KeyInput::Key k ) = 0;
  virtual int firstKeyPressed() const = 0;
  virtual void updateMapping( KeyInput::Key k, int code ) = 0;
};

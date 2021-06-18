#pragma once

struct KeyInput
{
  bool left;
  bool up;
  bool right;
  bool down;

  bool opt1;
  bool pause;
  bool opt2;

  bool a;
  bool b;
};

class IInputSource
{
public:
  virtual ~IInputSource() = default;

  virtual KeyInput getInput() const = 0;
};

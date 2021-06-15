#pragma once

class ComLynxWire
{
public:
  ComLynxWire() : mValue{ 0 }, mClients{ 0 } {}

  void pullUp()
  {
    mValue += 1;
  }

  void pullDown()
  {
    mValue -= 1;
  }

  int value() const
  {
    return mValue == 0 ? 1 : 0;
  }

  int connect()
  {
    return mClients++;
  }

private:
  //value is pulled up in idle (0) state.
  int mValue;
  int mClients;
};

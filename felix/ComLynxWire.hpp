#pragma once

class ComLynxWire
{
public:
  ComLynxWire() : mValue{ 0 }, mClients{ 0 }, mCoarseValue{}, mParBit{} {}

  void pullUp()
  {
    mValue += 1;
  }

  void pullDown()
  {
    mValue -= 1;
  }

  int wire() const
  {
    return mValue;
  }

  int value() const
  {
    return mValue == 0 ? 1 : 0;
  }

  int connect()
  {
    return mClients++;
  }

  void setCoarse( int value, int parbit )
  {
    mCoarseValue = value;
    mParBit = parbit;
  }

  int getCoarse( int & parbit ) const
  {
    return mCoarseValue;
    parbit = mParBit;
  }

private:
  //value is pulled up in idle (0) state.
  int mValue;
  int mClients;
  int mCoarseValue;
  int mParBit;
};

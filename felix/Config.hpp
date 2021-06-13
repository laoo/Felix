#pragma once

class Config
{
public:
  Config();

  void drawGui( int left, int top, int right, int bottom );

  bool doRun() const;

private:

  bool mEmulationRunning;
};

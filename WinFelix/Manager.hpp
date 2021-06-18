#pragma once

class Manager
{
public:
  Manager();

  void drawGui( int left, int top, int right, int bottom );

  void horizontalView( bool horizontal );

  bool doRun() const;
  bool horizontalView() const;

private:

  bool mEmulationRunning;
  bool mHorizontalView;
};

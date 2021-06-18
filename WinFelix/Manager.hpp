#pragma once

#include "IInputSource.hpp"

class Manager
{
public:
  Manager();

  void drawGui( int left, int top, int right, int bottom );

  void horizontalView( bool horizontal );

  bool doRun() const;
  bool horizontalView() const;

  void processKeys();

  std::shared_ptr<IInputSource> getInputSource( int instance );

private:

  struct InputSource : public IInputSource, public KeyInput
  {
    InputSource();
    ~InputSource() override = default;

    KeyInput getInput() const override;
  };

  bool mEmulationRunning;
  bool mHorizontalView;

  std::array<std::shared_ptr<InputSource>, 2> mIntputSources;
};

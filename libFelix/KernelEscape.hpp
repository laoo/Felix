#pragma once

#include "IEscape.hpp"

class TraceHelper;

class KernelEscape : public IEscape
{
public:
  KernelEscape( std::shared_ptr<TraceHelper> traceHelper );
  ~KernelEscape() override = default;

  void call( uint8_t data, IAccessor & acc ) override;

private:
  void shift( IAccessor & acc );
  void clear( IAccessor & acc );
  void decryptCartridge( IAccessor & acc );
  void reset( IAccessor & acc );

  std::shared_ptr<TraceHelper> mTraceHelper;
};

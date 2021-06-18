#pragma once

#include "IEscape.hpp"

class KernelEscape : public IEscape
{
public:
  KernelEscape();
  ~KernelEscape() override = default;

  void call( uint8_t data, IAccessor & acc ) override;

private:
  void shift( IAccessor & acc );
  void clear( IAccessor & acc );
  void decryptCartridge( IAccessor & acc );
  void reset( IAccessor & acc );
};

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
  void decrypt( IAccessor & acc );
  void reset( IAccessor & acc );

  std::vector<uint8_t> decrypt( size_t blockcount, std::span<uint8_t const> encrypted );
  void decrypt( std::span<uint8_t const> encrypted, int & accumulator, std::vector<uint8_t> & result );

};

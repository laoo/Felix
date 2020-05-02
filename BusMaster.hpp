#pragma once

#include <array>
#include <cstdint>
#include "CPU.hpp"

class BusMaster
{
  std::array<uint8_t, 65536> mMemory;
  CPURequest mReq;

public:
  BusMaster();

  CPURequest * request( Read r );
  CPURequest * request( ReadOpcode r );
  CPURequest * request( Write w );
  void process();

};

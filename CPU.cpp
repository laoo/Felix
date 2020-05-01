#include "CPU.hpp"
#include "BusMaster.hpp"

CpuLoop cpuLoop( CPU & cpu )
{
  co_yield{ 1234, 42 };
  uint8_t b = co_yield{ 1234 };
  co_yield{ 1235, b };
}


AwaitRead CpuLoop::promise_type::yield_value( Read r )
{
  return AwaitRead{ mBus->request( r ) };
}

AwaitWrite CpuLoop::promise_type::yield_value( Write w )
{
  return AwaitWrite{ mBus->request( w ) };
}

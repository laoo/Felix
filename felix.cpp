#include <iostream>
#include <experimental/coroutine>
#include "BusMaster.hpp"
#include "CPUExecute.hpp"
#include "CPUTrace.hpp"

int main()
{
  BusMaster bus;
  CPU cpu;
  CpuLoop loop = cpuLoop( cpu );
  CpuTrace disasm = cpuTrace( cpu, bus.getTraceRequest() );
  loop.setBusMaster( &bus );
  for ( ;; )
  {
    bus.process( 16000000 / 60 );
  }
  return 0;
}

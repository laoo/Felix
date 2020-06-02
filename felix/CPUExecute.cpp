#include "CPUExecute.hpp"
#include "Felix.hpp"

AwaitCPURead CpuExecute::promise_type::await_transform( CPURead r )
{
  return AwaitCPURead{ mFelix->request( r ) };
}

AwaitCPUFetchOpcode CpuExecute::promise_type::await_transform( CPUFetchOpcode r )
{
  return AwaitCPUFetchOpcode{ mFelix->request( r ) };
}

AwaitCPUFetchOperand CpuExecute::promise_type::await_transform( CPUFetchOperand r )
{
  return AwaitCPUFetchOperand{ mFelix->request( r ) };
}

AwaitCPUWrite CpuExecute::promise_type::await_transform( CPUWrite w )
{
  return AwaitCPUWrite{ mFelix->request( w ) };
}


AwaitCPUBusMaster CpuExecute::promise_type::await_transform( Felix & felix )
{
  mFelix = &felix;
  return AwaitCPUBusMaster{ mFelix->cpuRequest() };
}

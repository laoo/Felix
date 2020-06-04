#include "CPUExecute.hpp"
#include "MasterBus.hpp"
#include <stdexcept>

CpuExecute::CpuExecute( handle c )
{
  auto & res = MasterBus::instance().cpuResponse();

  if ( res.target )
    throw std::exception{};

  res.target = c;
}

CpuExecute::~CpuExecute()
{
  auto & res = MasterBus::instance().cpuResponse();
  
  if ( res.target )
  {
    res.target.destroy();
    res.target = {};
  }
}

bool CPUFetchOpcodeAwaiter::await_ready()
{
  return false;
}

OpInt CPUFetchOpcodeAwaiter::await_resume()
{
  auto & res = MasterBus::instance().cpuResponse();

  return { res.tick, res.interrupt, ( Opcode )res.value };
}

void CPUFetchOpcodeAwaiter::await_suspend( std::experimental::coroutine_handle<> c )
{
}

bool CPUFetchOperandAwaiter::await_ready()
{
  return false;
}

uint8_t CPUFetchOperandAwaiter::await_resume()
{
  auto & res = MasterBus::instance().cpuResponse();

  return res.value;
}

void CPUFetchOperandAwaiter::await_suspend( std::experimental::coroutine_handle<> c )
{
}

bool CPUReadAwaiter::await_ready()
{
  return false;
}

uint8_t CPUReadAwaiter::await_resume()
{
  auto & res = MasterBus::instance().cpuResponse();

  return res.value;
}

void CPUReadAwaiter::await_suspend( std::experimental::coroutine_handle<> c )
{
}

bool CPUWriteAwaiter::await_ready()
{
  return false;
}

void CPUWriteAwaiter::await_resume()
{
}

void CPUWriteAwaiter::await_suspend( std::experimental::coroutine_handle<> c )
{
}

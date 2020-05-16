#include "SuzyExecute.hpp"
#include "BusMaster.hpp"

void SuzyExecute::promise_type::return_void()
{
  mBus->suzyRequest()->raw = 0;
}

AwaitBusMaster SuzyExecute::promise_type::await_transform( BusMaster & bus )
{
  mBus = &bus;
  return AwaitBusMaster{ mBus->suzyRequest() };
}

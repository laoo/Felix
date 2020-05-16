#include "SuzyExecute.hpp"
#include "BusMaster.hpp"

void SuzyExecute::promise_type::return_void()
{
  mReq->raw = 0;
}

AwaitSuzyRequest SuzyExecute::promise_type::await_transform( SuzyRequest & req )
{
  mReq = &req;
  return AwaitSuzyRequest{ &req };
}

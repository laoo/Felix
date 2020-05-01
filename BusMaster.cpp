#include "BusMaster.hpp"
#include "CPU.hpp"

BusMaster::BusMaster() : mMemory{}
{
}

CPURequest * BusMaster::request( Read r )
{
  mReq = CPURequest{ r };
  return &mReq;
}

CPURequest * BusMaster::request( Write w )
{
  mReq = CPURequest{ w };
  return &mReq;
}

void BusMaster::process()
{
  for ( ;; )
  {
    switch ( mReq.mType )
    {
    case CPURequest::Type::READ:
      mReq.value = mMemory[mReq.address];
      mReq.resume();
      break;
    case CPURequest::Type::WRITE:
      mMemory[mReq.address] = mReq.value;
      mReq.resume();
      break;
    default:
      return;
    }
  }
}

#include "MasterBus.hpp"


MasterBus::MasterBus() : mCPURequest{}, mCPUResponse{}
{

}

MasterBus & MasterBus::instance()
{
  static MasterBus bus{};

  return bus;
}

MasterBus::CPURequest & MasterBus::cpuRequest()
{
  return mCPURequest;
}

MasterBus::CPUResponse & MasterBus::cpuResponse()
{
  return mCPUResponse;
}

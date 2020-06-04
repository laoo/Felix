#pragma once

#include <cstdint>
#include <experimental/coroutine>

namespace detail
{
template <class T>
class NonCopyable
{
public:
  NonCopyable( const NonCopyable & ) = delete;
  T & operator = ( const T & ) = delete;

protected:
  NonCopyable() = default;
  ~NonCopyable() = default;
};
}

class MasterBus : private detail::NonCopyable<MasterBus>
{
private:
  MasterBus();

public:

  static MasterBus & instance();


  struct CPURequest : private detail::NonCopyable<CPURequest>
  {
    enum class Type : uint8_t
    {
      NONE,
      FETCH_OPCODE,
      FETCH_OPERAND,
      READ,
      WRITE,
    };

    uint16_t address;
    uint8_t value;
    Type type;
  };

  struct CPUResponse : private detail::NonCopyable<CPUResponse>
  {
    uint64_t tick;
    int interrupt;
    uint8_t value;
    std::experimental::coroutine_handle<> target;
  };

public:


  CPURequest & cpuRequest();
  CPUResponse & cpuResponse();

private:

  CPURequest mCPURequest;
  CPUResponse mCPUResponse;


};

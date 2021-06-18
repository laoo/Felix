#pragma once

class NonCopyable
{
public:
  NonCopyable( NonCopyable const& ) = delete;
  NonCopyable& operator= (  NonCopyable const& ) = delete;

protected:
  NonCopyable() = default;
  ~NonCopyable() = default;
};

struct AudioSample
{
  int16_t left;
  int16_t right;
};


#ifdef __cpp_lib_bitops

#define popcnt(X) std::popcount(X)

#else

uint32_t popcnt( uint32_t x )
{
  int v = 0;
  while ( x != 0 )
  {
    x &= x - 1;
    v++;
  }
  return v;
}

#endif
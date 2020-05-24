#pragma once
#include "Suzy.hpp"
#include <array>

class VidOperator
{
public:
  struct MemOp
  {
    uint16_t addr;
    uint8_t value;
    enum class Op : uint8_t
    {
      NONE,
      READ,
      WRITE,
    } op;

    explicit operator bool() const
    {
      return op != Op::NONE;
    }
    operator Op()
    {
      return op;
    }
  };

  struct State
  {
    int v;
  };

public:
  VidOperator();

  MemOp flush();

  typedef MemOp( *StateFuncT )( VidOperator::State & s );

  static constexpr size_t STATEFUN_SIZE = 1 << 6;

private:

  std::array<StateFuncT, STATEFUN_SIZE> mStateFuncs;

};

/*

typedef int (*funct)();

template<int V>
constexpr int fun()
{
    if constexpr ( V == 3 )
        return 1;
    else return V + 3;
}

template<int... I>
constexpr std::array<funct, 5> make(std::integer_sequence<int, I...>)
{
    return { fun<I>... };
}

int main( int argc, char** argv )
{
    static constexpr std::array<funct, 5> a{ make( std::make_integer_sequence<int, 5>{} ) };
    return a[argc]();
}

*/
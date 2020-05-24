#include "VidOperator.hpp"
#include <utility>

template<int V>
VidOperator::MemOp stateFun( VidOperator::State & s )
{
  if constexpr ( V == 42 )
    s.v = V;
  else
    s.v = V * 2;

  return VidOperator::MemOp{};
}

template<int... I>
static std::array<VidOperator::StateFuncT, VidOperator::STATEFUN_SIZE> makeStateFunc( std::integer_sequence<int, I...> )
{
  return { stateFun<I>... };

}


VidOperator::VidOperator() : mStateFuncs{ makeStateFunc( std::make_integer_sequence<int, VidOperator::STATEFUN_SIZE>{} ) }
{
}

VidOperator::MemOp VidOperator::flush()
{
  return { 0, 0, MemOp::Op::NONE };
}

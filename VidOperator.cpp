#include "VidOperator.hpp"

VidOperator::VidOperator()
{
}

VidOperator::MemOp VidOperator::flush()
{
  return { 0, 0, MemOp::Op::NONE };
}

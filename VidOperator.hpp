#pragma once
#include "Suzy.hpp"

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

public:
  VidOperator();

  MemOp flush();

};

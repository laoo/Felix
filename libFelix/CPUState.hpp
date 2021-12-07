#pragma once

#include "Opcodes.hpp"

struct CPUState
{
  static constexpr int I_NONE = 0;
  static constexpr int I_IRQ = 1;
  static constexpr int I_NMI = 2;
  static constexpr int I_RESET = 4;

  CPUState() : tick{}, interrupt{ I_RESET }, op{}, pc{}, s{ 0x1ff }, a{}, x{}, y{}, p{}, ea{}, fa{}, t{}, m1{}, m2{} {}

  uint64_t tick;
  uint8_t interrupt;
  Opcode op;
  union
  {
    uint16_t pc;
    struct
    {
      uint8_t pcl;
      uint8_t pch;
    };
  };
  union
  {
    uint16_t s;
    struct
    {
      uint8_t sl;
      uint8_t sh;
    };
  };
  uint8_t a;
  uint8_t x;
  uint8_t y;
  uint8_t p;

  union
  {
    uint16_t ea{};
    struct
    {
      uint8_t eal;
      uint8_t eah;
    };
  };

  union
  {
    uint16_t fa{};
    struct
    {
      uint8_t fal;
      uint8_t fah;
    };
  };

  union
  {
    uint16_t t;
    struct
    {
      uint8_t tl;
      uint8_t th;
    };
  };

  uint8_t m1;
  uint8_t m2;

};
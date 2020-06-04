#pragma once
#include <cstdint>
#include "CPUExecute.hpp"

enum class Opcode : uint8_t;

struct CPU
{
  static constexpr int bitC = 0;
  static constexpr int bitZ = 1;
  static constexpr int bitI = 2;
  static constexpr int bitD = 3;
  static constexpr int bitB = 4;
  static constexpr int bitV = 6;
  static constexpr int bitN = 7;

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
  uint8_t P;

  static const int I_NONE  = 0;
  static const int I_IRQ = 1;
  static const int I_NMI = 2;
  static const int I_RESET = 4;

  CPU() : pc{}, s{ 0x1ff }, a{}, x{}, y{}, P{}, tick{}, interrupt{ I_RESET }, opcode{}, operand{}
  {
  }

  uint8_t p() const
  {
    return P | 0x30;
  }

  uint8_t pirq() const
  {
    return P | 0x20;
  }

  void setnz( uint8_t v )
  {
    set<bitZ>( v == 0 );
    set<bitN>( v >= 0x80 );
  }

  void setz( uint8_t v )
  {
    set<bitZ>( v == 0 );
  }

  template<int bit>
  void set()
  {
    P |= 1 << bit;
  }

  template<int bit>
  void set( bool value )
  {
    value ? set<bit>() : clear<bit>();
  }

  template<int bit>
  void clear()
  {
    P &= ~( 1 << bit );
  }

  template<int bit>
  void clear( bool value )
  {
    value ? clear<bit>() : set<bit>();
  }

  template<int bit>
  bool get() const
  {
    return ( P & ( 1 << bit ) ) != 0;
  }

  void asl( uint8_t & val );
  void lsr( uint8_t & val );
  void rol( uint8_t & val );
  void ror( uint8_t & val );
  bool executeCommon( Opcode opcode, uint8_t value );

  CpuExecute execute();


  uint64_t tick;
  int interrupt;
  Opcode opcode;
  uint8_t operand;

};


#pragma once
#include <cstdint>

//stolen form http://www.maizure.org/projects/decoded-bisqwit-nes-emulator/nesemu1_lines.txt
template< unsigned bitno, typename T = uint8_t>
struct RegBit
{
  T data;
  template < typename T2>
  RegBit & operator=( T2 val )
  {
    data = ( data & ~( 1 << bitno ) ) | ( ( val & 1 ) << bitno );
    return *this;
  }
  operator unsigned() const
  {
    return ( data >> bitno ) & 1;
  }

  explicit operator bool () const
  {
    return ( ( data >> bitno ) & 1 ) != 0;
  }

};

enum class Opcode : uint8_t;

struct CPU
{
  uint8_t a;
  uint8_t x;
  uint8_t y;
  union
  {
    uint16_t s;
    struct
    {
      uint8_t sl;
      uint8_t sh;
    };
  };
  union
  {
    uint8_t P;
    RegBit<0> C;
    RegBit<1> Z;
    RegBit<2> I;
    RegBit<3> D;
    RegBit<6> V;
    RegBit<7> N;
  };
  union
  {
    uint16_t pc;
    struct
    {
      uint8_t pcl;
      uint8_t pch;
    };
  };

  static const int I_NONE  = 0;
  static const int I_IRQ = 1;
  static const int I_NMI = 2;
  static const int I_RESET = 4;

  CPU() : a{}, x{}, y{}, s{ 0x1ff }, pc{}, P{}
  {
  }

  uint8_t p() const
  {
    return P | 0x20;
  }

  uint8_t pbrk() const
  {
    return P | 0x30;
  }

  void setnz( uint8_t v )
  {
    Z = v == 0 ? 1 : 0;
    N = v >= 80 ? 1 : 0;
  }

  void setz( uint8_t v )
  {
    Z = v == 0 ? 1 : 0;
  }

  void asl( uint8_t & val );
  void lsr( uint8_t & val );
  void rol( uint8_t & val );
  void ror( uint8_t & val );
  bool executeR( Opcode opcode, uint8_t value );


private:

};


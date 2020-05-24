#pragma once
#include "Suzy.hpp"
#include <array>

class VidOperator
{
public:
  struct MemOp
  {
    static constexpr uint8_t LEFT  = 0b01000;
    static constexpr uint8_t RIGHT = 0b10000;
    static constexpr uint8_t WRITE = 0b00001;
    static constexpr uint8_t MODIFY= 0b00010;
    static constexpr uint8_t XOR   = 0b00100;

    union
    {
      struct
      {
        uint16_t addr;
        uint8_t value;
        uint8_t op;
      };
      uint32_t word;
    };
 
    uint8_t mask() const
    {
      switch ( op & ( LEFT | RIGHT ) )
      {
      case RIGHT:
        return 0xf0;
      case LEFT:
        return 0x0f;
      case LEFT | RIGHT:
        return 0x00;
      default:
        return 0xff;
      }
    }

    operator int() const
    {
      return op & ( WRITE | MODIFY | XOR );
    }

  };
 
public:
  VidOperator();

  MemOp flush();

  void writeLeft( int pixel );
  void rmwLeft( int pixel );
  void writeRight( int pixel );
  void rmwRight( int pixel );
  void eor( int pixel );

  typedef void( *StateFuncT )( VidOperator & vop );

  static constexpr size_t STATEFUN_SIZE = 1 << 6;

private:

  std::array<StateFuncT, STATEFUN_SIZE> mStateFuncs;
  MemOp mOp;

};

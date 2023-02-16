#pragma once
#include "Suzy.hpp"

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
        uint8_t value;
        uint8_t op;
        uint16_t addr;
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

  static_assert( sizeof( MemOp ) == sizeof( uint32_t ) );
 
public:
  VidOperator( Suzy::Sprite spriteType );

  MemOp flush();
  void newLine( uint16_t vidadr );
  MemOp process( int hpos, uint8_t pixel );

  static constexpr size_t STATEFUN_SIZE = 1 << 6;

private:
  static const std::array<MemOp, STATEFUN_SIZE*8> mStateFuncs;
  int mSpriteType;
  int mOff;
  MemOp mOp;
  uint16_t mVidAdr;
  uint8_t mEdge;

};

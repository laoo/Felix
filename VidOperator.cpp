#include "VidOperator.hpp"
#include "Suzy.hpp"
#include <utility>

namespace
{

struct SuzySpriteBACKGROUND
{
  static constexpr bool eor = false;
  static constexpr bool background = true;

  static constexpr bool opaque( int pixel )
  {
    return true;
  }

};

struct SuzySpriteBACKNONCOLL
{
  static constexpr bool eor = false;
  static constexpr bool background = true;

  static constexpr bool opaque( int pixel )
  {
    return true;
  }
};

struct SuzySpriteBSHADOW
{
  static constexpr bool eor = false;
  static constexpr bool background = false;

  static constexpr bool opaque( int pixel )
  {
    return pixel != 0x0 && pixel != 0xf;
  }

};

struct SuzySpriteBOUNDARY
{
  static constexpr bool eor = false;
  static constexpr bool background = false;

  static constexpr bool opaque( int pixel )
  {
    return pixel != 0x0 && pixel != 0xf;
  }
};

struct SuzySpriteNORMAL
{
  static constexpr bool eor = false;
  static constexpr bool background = false;

  static constexpr bool opaque( int pixel )
  {
    return pixel != 0x0;
  }

};

struct SuzySpriteNONCOLL
{
  static constexpr bool eor = false;
  static constexpr bool background = false;

  static constexpr bool opaque( int pixel )
  {
    return pixel != 0x0;
  }

};

struct SuzySpriteXOR
{
  static constexpr bool eor = true;
};

struct SuzySpriteSHADOW
{
  static constexpr bool eor = false;
  static constexpr bool background = false;

  static constexpr bool opaque( int pixel )
  {
    return pixel != 0x0;
  }
};

}

template<typename Type, int I>
constexpr VidOperator::MemOp stateFun()
{
  static constexpr int pixel = ( I & 0b111100 ) >> 2;  //pixel value
  static constexpr bool edge = ( I & 0b000010 ) != 0;  //first pixel
  static constexpr bool even = ( I & 0b000001 ) != 0;  //right nibble
 
  static constexpr int value = even ? pixel : pixel << 4;
 
  if constexpr ( Type::eor )
  {
    return VidOperator::MemOp{ ( uint8_t )( value ), VidOperator::MemOp::XOR };
  }
  else
  {
    static constexpr bool opaque = Type::opaque( pixel );
    if constexpr ( opaque )
    {
      if constexpr ( edge )
      {
        return VidOperator::MemOp{ ( uint8_t )( value ),
          VidOperator::MemOp::MODIFY | ( even ? VidOperator::MemOp::RIGHT : VidOperator::MemOp::LEFT )
        };
      }
      else
      {
        return VidOperator::MemOp{ ( uint8_t )( value ),
          ( Type::background ? VidOperator::MemOp::WRITE : VidOperator::MemOp::MODIFY ) |
          ( even ? VidOperator::MemOp::RIGHT : VidOperator::MemOp::LEFT )
        };
      }
    }
    else
    {
      return VidOperator::MemOp{};
    }
    //normal
  }
}

template<typename Type, int... I>
static std::array<VidOperator::MemOp, VidOperator::STATEFUN_SIZE> makeStateFunc( std::integer_sequence<int, I...> )
{
  return { stateFun<Type,I>()... };

}

VidOperator::VidOperator( Suzy::Sprite spriteType ) : mStateFuncs{}, mOff{}, mOp{}, mVidAdr{}, mEdge{}
{
  switch ( spriteType )
  {
  case Suzy::Sprite::BACKGROUND:
    mStateFuncs ={ makeStateFunc<SuzySpriteBACKGROUND>( std::make_integer_sequence<int, VidOperator::STATEFUN_SIZE>{} ) };
    break;
  case Suzy::Sprite::BACKNONCOLL:
    mStateFuncs ={ makeStateFunc<SuzySpriteBACKNONCOLL>( std::make_integer_sequence<int, VidOperator::STATEFUN_SIZE>{} ) };
    break;
  case Suzy::Sprite::BSHADOW:
    mStateFuncs ={ makeStateFunc<SuzySpriteBSHADOW>( std::make_integer_sequence<int, VidOperator::STATEFUN_SIZE>{} ) };
    break;
  case Suzy::Sprite::BOUNDARY:
    mStateFuncs ={ makeStateFunc<SuzySpriteBOUNDARY>( std::make_integer_sequence<int, VidOperator::STATEFUN_SIZE>{} ) };
    break;
  case Suzy::Sprite::NORMAL:
    mStateFuncs ={ makeStateFunc<SuzySpriteNORMAL>( std::make_integer_sequence<int, VidOperator::STATEFUN_SIZE>{} ) };
    break;
  case Suzy::Sprite::NONCOLL:
    mStateFuncs ={ makeStateFunc<SuzySpriteNONCOLL>( std::make_integer_sequence<int, VidOperator::STATEFUN_SIZE>{} ) };
    break;
  case Suzy::Sprite::XOR:
    mStateFuncs ={ makeStateFunc<SuzySpriteXOR>( std::make_integer_sequence<int, VidOperator::STATEFUN_SIZE>{} ) };
    break;
  case Suzy::Sprite::SHADOW:
    mStateFuncs ={ makeStateFunc<SuzySpriteSHADOW>( std::make_integer_sequence<int, VidOperator::STATEFUN_SIZE>{} ) };
    break;
  }
}

VidOperator::MemOp VidOperator::flush()
{
  mOp.addr = mVidAdr + mOff;
  mOp.op |= MemOp::MODIFY;
  return mOp;
}

void VidOperator::newLine( uint16_t vidadr )
{
  mVidAdr = vidadr;
  mOp = MemOp{};
  mEdge = 2;
}

VidOperator::MemOp VidOperator::process( int hpos, uint8_t pixel )
{
  int off = hpos >> 1;
  if ( mEdge )
  {
    mOff = off;
    int idx = ( pixel << 2 ) | mEdge | ( hpos & 1 );
    mOp = mStateFuncs[idx];
    mEdge = 0;
    return MemOp{};
  }
  else
  {
    int idx = ( pixel << 2 ) | ( hpos & 1 );
    if ( mOff == off )
    {
      auto op = mStateFuncs[idx];
      mOp.word |= op.word;
      return MemOp{};
    }
    else
    {
      MemOp result ={ mOp.value, mOp.op, (uint16_t)( mVidAdr + mOff ) };
      mOff = off;
      mOp = mStateFuncs[idx];
      return result;
    }
  }
}


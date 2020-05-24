#include "VidOperator.hpp"
#include <utility>

namespace
{

struct SuzySpriteBACKGROUND
{
  static constexpr bool eor = false;

  static constexpr bool opaque( int pixel )
  {
    return true;
  }

};

struct SuzySpriteBACKNONCOLL
{
  static constexpr bool eor = false;

  static constexpr bool opaque( int pixel )
  {
    return true;
  }
};

struct SuzySpriteBSHADOW
{
  static constexpr bool eor = false;

  static constexpr bool opaque( int pixel )
  {
    return pixel != 0x0 && pixel != 0xf;
  }

};

struct SuzySpriteBOUNDARY
{
  static constexpr bool eor = false;

  static constexpr bool opaque( int pixel )
  {
    return pixel != 0x0 && pixel != 0xf;
  }
};

struct SuzySpriteNORMAL
{
  static constexpr bool eor = false;

  static constexpr bool opaque( int pixel )
  {
    return pixel != 0x0;
  }

};

struct SuzySpriteNONCOLL
{
  static constexpr bool eor = false;

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

  static constexpr bool opaque( int pixel )
  {
    return pixel != 0x0;
  }
};

}

template<typename Type, int I>
void stateFun( VidOperator & vop )
{
  static constexpr int pixel = ( I & 0b111100 ) >> 2;  //pixel value
  static constexpr bool even = ( I & 0b000010 ) != 0;  //right nibble
  static constexpr bool left = ( I & 0b000001 ) != 0;  //moving from right to left

  static constexpr bool opaque = Type::opaque( pixel );
  static constexpr int value = even ? pixel : pixel << 4;

 
  if constexpr ( Type::eor )
  {
    //eor;
  }
  else
  {
    //normal
  }
}

template<typename Type, int... I>
static std::array<VidOperator::StateFuncT, VidOperator::STATEFUN_SIZE> makeStateFunc( std::integer_sequence<int, I...> )
{
  return { stateFun<Type,I>... };

}

VidOperator::VidOperator() : mStateFuncs { makeStateFunc<SuzySpriteBACKNONCOLL>( std::make_integer_sequence<int, VidOperator::STATEFUN_SIZE>{} ) },
  mOp{}
{
}

VidOperator::MemOp VidOperator::flush()
{
  MemOp result = mOp;
  mOp.word = 0;
  return mOp;
}

void VidOperator::writeLeft( int pixel )
{
  assert( ( pixel & 0x0f ) == 0 );
  mOp.value = mOp.value & 0x0f | pixel;
  mOp.op |= MemOp::WRITE | MemOp::LEFT;
}

void VidOperator::rmwLeft( int pixel )
{
  assert( ( pixel & 0x0f ) == 0 );
  mOp.value = mOp.value & 0x0f | pixel;
  mOp.op = MemOp::MODIFY | MemOp::LEFT;
}

void VidOperator::writeRight( int pixel )
{
  assert( ( pixel & 0xf0 ) == 0 );
  mOp.value = mOp.value & 0xf0 | pixel;
  mOp.op |= MemOp::WRITE | MemOp::RIGHT;
}

void VidOperator::rmwRight( int pixel )
{
  assert( ( pixel & 0xf0 ) == 0 );
  mOp.value = mOp.value & 0xf0 | pixel;
  mOp.op |= MemOp::MODIFY | MemOp::RIGHT;
}

void VidOperator::eor( int pixel )
{
  mOp.value ^= pixel;
  mOp.op |= MemOp::XOR;
}


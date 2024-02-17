#include "VidOperator.hpp"
#include "SpriteTemplates.hpp"

template<typename Type, int I> requires requires
{
  { Type::eor } -> std::convertible_to<bool>;
  { Type::background } -> std::convertible_to<bool>;
  { Type::opaque( 0 ) } -> std::convertible_to<bool>;
}
constexpr VidOperator::MemOp stateFun()
{
  constexpr int pixel = ( I & 0b111100 ) >> 2;  //pixel value
  constexpr bool edge = ( I & 0b000010 ) != 0;  //first pixel
  constexpr bool even = ( I & 0b000001 ) != 0;  //right nibble
 
  constexpr int value = even ? pixel : pixel << 4;
 
  if constexpr ( Type::eor )
  {
    return VidOperator::MemOp{ ( uint8_t )( value ), VidOperator::MemOp::XOR };
  }
  else
  {
    constexpr bool opaque = Type::opaque( pixel );
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

template<int... I>
static std::array<VidOperator::MemOp, VidOperator::STATEFUN_SIZE * 8> makeStateFunc( std::integer_sequence<int, I...> )
{
  return { stateFun<SuzySprite<(Suzy::Sprite)( I >> 6 )>,I>()... };
}

const std::array<VidOperator::MemOp, VidOperator::STATEFUN_SIZE * 8> VidOperator::mStateFuncs = makeStateFunc( std::make_integer_sequence<int, VidOperator::STATEFUN_SIZE * 8>{} );

VidOperator::VidOperator( Suzy::Sprite spriteType ) :
  mSpriteType{ (int)spriteType }, mOff{}, mOp{}, mVidAdr{}, mEdge{}
{
}

VidOperator::MemOp VidOperator::flush()
{
  mOp.addr = mVidAdr + mOff;
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
    int idx = mSpriteType * VidOperator::STATEFUN_SIZE | ( pixel << 2 ) | mEdge | ( hpos & 1 );
    mOp = mStateFuncs[idx];
    mEdge = 0;
    return MemOp{};
  }
  else
  {
    int idx = mSpriteType * VidOperator::STATEFUN_SIZE | ( pixel << 2 ) | ( hpos & 1 );
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


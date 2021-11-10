#include "pch.hpp"
#include "ColOperator.hpp"
#include "SpriteTemplates.hpp"
#include "Log.hpp"
namespace
{

template<typename Type, int PIXEL>
constexpr bool processFun()
{
  return Type::colliding( PIXEL ) && Type::collWrite;
}

template<int... I>
constexpr std::array<bool, ColOperator::STATES_SIZE> makeProcessStates( std::integer_sequence<int, I...> )
{
  return { processFun<SuzySprite<( Suzy::Sprite )( I / ColOperator::POSSIBLE_PIXELS )>, I % ColOperator::POSSIBLE_PIXELS>()... };
}

template<int... I>
constexpr std::array<bool, 8> makeSpriteCollideable( std::integer_sequence<int, I...> )
{
  return { SuzySprite<( Suzy::Sprite )I>::collWrite... };
}

template<int... I>
constexpr std::array<bool, 8> makeDepositoryUpdatable( std::integer_sequence<int, I...> )
{
  return { SuzySprite<( Suzy::Sprite )I>::collDep... };
}

//processing functions for each sprite type
static constexpr std::array<bool, ColOperator::STATES_SIZE> procesStates = makeProcessStates( std::make_integer_sequence<int, ColOperator::STATES_SIZE>{} );
//whether each sprite type is collideable
static constexpr std::array<bool, ColOperator::SPRITE_TYPES> spriteCollideable = makeSpriteCollideable( std::make_integer_sequence<int, 8>{} );
//whether collision depository is updatable for each sprite type
static constexpr std::array<bool, ColOperator::SPRITE_TYPES> depositoryUpdatable = makeDepositoryUpdatable( std::make_integer_sequence<int, 8>{} );
}


ColOperator::ColOperator( Suzy::Sprite spriteType, uint8_t sprColl ) : mSpriteType{ (size_t)spriteType }, mMask{}, mHiColl{}, mStoreOff{}, mColAdr{}, mColl{ (uint16_t)( ( sprColl & 0xf ) | ( ( sprColl & 0xf ) << 4 ) ) }
{
  mColl |= mColl << 8;
}

ColOperator::MemOp ColOperator::flush()
{
  if ( mMask )
  {
    return MemOp{ mMask, ( uint16_t )( mColAdr + mStoreOff ), mColl };
  }
  else
  {
    return {};
  }
}

void ColOperator::newLine( uint16_t coladr )
{
  mColAdr = coladr;
  mStoreOff = ~0;  //invalid address to detect first access
  mMask = 0x0000;
}

ColOperator::MemOp ColOperator::process( int hpos, uint8_t pixel )
{
  MemOp result{};

  int32_t hposfloor = ( hpos & ~7 ) >> 1;
  int32_t hposrem = ( hpos & 7 ) << 2;

  if ( mStoreOff != hposfloor )
  {
    if ( mMask )
    {
      result = MemOp{ mMask, (uint16_t)( mColAdr + mStoreOff ), mColl };
      mMask = 0;
    }
    mStoreOff = hposfloor;
  }

  assert( pixel < ColOperator::POSSIBLE_PIXELS );
  size_t idx = mSpriteType * ColOperator::POSSIBLE_PIXELS + pixel;
  if ( procesStates[idx] )
  {
    mMask |= 0xf0000000 >> hposrem;
  }
 
  return result;
}

void ColOperator::receiveHiColl( uint32_t value )
{
  if ( depositoryUpdatable[mSpriteType] )
  {
    auto v7 = value & 0xf0000000;
    auto v6 = value & 0x0f000000;
    auto v5 = value & 0x00f00000;
    auto v4 = value & 0x000f0000;
    auto v3 = value & 0x0000f000;
    auto v2 = value & 0x00000f00;
    auto v1 = value & 0x000000f0;
    auto v0 = value & 0x0000000f;

    auto t7 = mHiColl & 0xf0000000;
    auto t6 = mHiColl & 0x0f000000;
    auto t5 = mHiColl & 0x00f00000;
    auto t4 = mHiColl & 0x000f0000;
    auto t3 = mHiColl & 0x0000f000;
    auto t2 = mHiColl & 0x00000f00;
    auto t1 = mHiColl & 0x000000f0;
    auto t0 = mHiColl & 0x0000000f;

    mHiColl =
      ( v7 > t7 ? v7 : t7 ) |
      ( v6 > t6 ? v6 : t6 ) |
      ( v5 > t5 ? v5 : t5 ) |
      ( v4 > t4 ? v4 : t4 ) |
      ( v3 > t3 ? v3 : t3 ) |
      ( v2 > t2 ? v2 : t2 ) |
      ( v1 > t1 ? v1 : t1 ) |
      ( v0 > t0 ? v0 : t0 );
  }
}

std::optional<uint8_t> ColOperator::hiColl() const
{
  if ( depositoryUpdatable[mSpriteType] )
  {
    auto t7 = ( mHiColl & 0xf0000000 ) >> 28;
    auto t6 = ( mHiColl & 0x0f000000 ) >> 24;
    auto t5 = ( mHiColl & 0x00f00000 ) >> 20;
    auto t4 = ( mHiColl & 0x000f0000 ) >> 16;
    auto t3 = ( mHiColl & 0x0000f000 ) >> 12;
    auto t2 = ( mHiColl & 0x00000f00 ) >> 8;
    auto t1 = ( mHiColl & 0x000000f0 ) >> 4;
    auto t0 = ( mHiColl & 0x0000000f ) >> 0;

    auto t76 = t7 > t6 ? t7 : t6;
    auto t54 = t5 > t4 ? t5 : t4;
    auto t32 = t3 > t2 ? t3 : t2;
    auto t10 = t1 > t0 ? t1 : t0;

    auto t7654 = t76 > t54 ? t76 : t54;
    auto t3210 = t32 > t10 ? t32 : t10;

    auto t76543210 = t7654 > t3210 ? t7654 : t3210;

    L_TRACE << "Deposit " << t76543210;
    return t76543210;
  }
  else
    return {};
}

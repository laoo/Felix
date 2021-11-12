#include "pch.hpp"
#include "ColOperator.hpp"
#include "SpriteTemplates.hpp"
#include "Log.hpp"

namespace
{

//whether collision buffer is written for each sprite pixel of each sprite type
static constexpr std::array<std::array<bool, ColOperator::POSSIBLE_PIXELS>, ColOperator::SPRITE_TYPES> statesTables
{
  std::array<bool, ColOperator::POSSIBLE_PIXELS>
  {
    SuzySprite<Suzy::Sprite::BACKGROUND>::colliding( 0x0 ),
    SuzySprite<Suzy::Sprite::BACKGROUND>::colliding( 0x1 ),
    SuzySprite<Suzy::Sprite::BACKGROUND>::colliding( 0x2 ),
    SuzySprite<Suzy::Sprite::BACKGROUND>::colliding( 0x3 ),
    SuzySprite<Suzy::Sprite::BACKGROUND>::colliding( 0x4 ),
    SuzySprite<Suzy::Sprite::BACKGROUND>::colliding( 0x5 ),
    SuzySprite<Suzy::Sprite::BACKGROUND>::colliding( 0x6 ),
    SuzySprite<Suzy::Sprite::BACKGROUND>::colliding( 0x7 ),
    SuzySprite<Suzy::Sprite::BACKGROUND>::colliding( 0x8 ),
    SuzySprite<Suzy::Sprite::BACKGROUND>::colliding( 0x9 ),
    SuzySprite<Suzy::Sprite::BACKGROUND>::colliding( 0xa ),
    SuzySprite<Suzy::Sprite::BACKGROUND>::colliding( 0xb ),
    SuzySprite<Suzy::Sprite::BACKGROUND>::colliding( 0xc ),
    SuzySprite<Suzy::Sprite::BACKGROUND>::colliding( 0xd ),
    SuzySprite<Suzy::Sprite::BACKGROUND>::colliding( 0xe ),
    SuzySprite<Suzy::Sprite::BACKGROUND>::colliding( 0xf )
  },
  std::array<bool, ColOperator::POSSIBLE_PIXELS>
  {
    SuzySprite<Suzy::Sprite::BACKNONCOLL>::colliding( 0x0 ),
    SuzySprite<Suzy::Sprite::BACKNONCOLL>::colliding( 0x1 ),
    SuzySprite<Suzy::Sprite::BACKNONCOLL>::colliding( 0x2 ),
    SuzySprite<Suzy::Sprite::BACKNONCOLL>::colliding( 0x3 ),
    SuzySprite<Suzy::Sprite::BACKNONCOLL>::colliding( 0x4 ),
    SuzySprite<Suzy::Sprite::BACKNONCOLL>::colliding( 0x5 ),
    SuzySprite<Suzy::Sprite::BACKNONCOLL>::colliding( 0x6 ),
    SuzySprite<Suzy::Sprite::BACKNONCOLL>::colliding( 0x7 ),
    SuzySprite<Suzy::Sprite::BACKNONCOLL>::colliding( 0x8 ),
    SuzySprite<Suzy::Sprite::BACKNONCOLL>::colliding( 0x9 ),
    SuzySprite<Suzy::Sprite::BACKNONCOLL>::colliding( 0xa ),
    SuzySprite<Suzy::Sprite::BACKNONCOLL>::colliding( 0xb ),
    SuzySprite<Suzy::Sprite::BACKNONCOLL>::colliding( 0xc ),
    SuzySprite<Suzy::Sprite::BACKNONCOLL>::colliding( 0xd ),
    SuzySprite<Suzy::Sprite::BACKNONCOLL>::colliding( 0xe ),
    SuzySprite<Suzy::Sprite::BACKNONCOLL>::colliding( 0xf )
  },
  std::array<bool, ColOperator::POSSIBLE_PIXELS>
  {
    SuzySprite<Suzy::Sprite::BSHADOW>::colliding( 0x0 ),
    SuzySprite<Suzy::Sprite::BSHADOW>::colliding( 0x1 ),
    SuzySprite<Suzy::Sprite::BSHADOW>::colliding( 0x2 ),
    SuzySprite<Suzy::Sprite::BSHADOW>::colliding( 0x3 ),
    SuzySprite<Suzy::Sprite::BSHADOW>::colliding( 0x4 ),
    SuzySprite<Suzy::Sprite::BSHADOW>::colliding( 0x5 ),
    SuzySprite<Suzy::Sprite::BSHADOW>::colliding( 0x6 ),
    SuzySprite<Suzy::Sprite::BSHADOW>::colliding( 0x7 ),
    SuzySprite<Suzy::Sprite::BSHADOW>::colliding( 0x8 ),
    SuzySprite<Suzy::Sprite::BSHADOW>::colliding( 0x9 ),
    SuzySprite<Suzy::Sprite::BSHADOW>::colliding( 0xa ),
    SuzySprite<Suzy::Sprite::BSHADOW>::colliding( 0xb ),
    SuzySprite<Suzy::Sprite::BSHADOW>::colliding( 0xc ),
    SuzySprite<Suzy::Sprite::BSHADOW>::colliding( 0xd ),
    SuzySprite<Suzy::Sprite::BSHADOW>::colliding( 0xe ),
    SuzySprite<Suzy::Sprite::BSHADOW>::colliding( 0xf )
  },
  std::array<bool, ColOperator::POSSIBLE_PIXELS>
  {
    SuzySprite<Suzy::Sprite::BOUNDARY>::colliding( 0x0 ),
    SuzySprite<Suzy::Sprite::BOUNDARY>::colliding( 0x1 ),
    SuzySprite<Suzy::Sprite::BOUNDARY>::colliding( 0x2 ),
    SuzySprite<Suzy::Sprite::BOUNDARY>::colliding( 0x3 ),
    SuzySprite<Suzy::Sprite::BOUNDARY>::colliding( 0x4 ),
    SuzySprite<Suzy::Sprite::BOUNDARY>::colliding( 0x5 ),
    SuzySprite<Suzy::Sprite::BOUNDARY>::colliding( 0x6 ),
    SuzySprite<Suzy::Sprite::BOUNDARY>::colliding( 0x7 ),
    SuzySprite<Suzy::Sprite::BOUNDARY>::colliding( 0x8 ),
    SuzySprite<Suzy::Sprite::BOUNDARY>::colliding( 0x9 ),
    SuzySprite<Suzy::Sprite::BOUNDARY>::colliding( 0xa ),
    SuzySprite<Suzy::Sprite::BOUNDARY>::colliding( 0xb ),
    SuzySprite<Suzy::Sprite::BOUNDARY>::colliding( 0xc ),
    SuzySprite<Suzy::Sprite::BOUNDARY>::colliding( 0xd ),
    SuzySprite<Suzy::Sprite::BOUNDARY>::colliding( 0xe ),
    SuzySprite<Suzy::Sprite::BOUNDARY>::colliding( 0xf )
  },
  std::array<bool, ColOperator::POSSIBLE_PIXELS>
  {
    SuzySprite<Suzy::Sprite::NORMAL>::colliding( 0x0 ),
    SuzySprite<Suzy::Sprite::NORMAL>::colliding( 0x1 ),
    SuzySprite<Suzy::Sprite::NORMAL>::colliding( 0x2 ),
    SuzySprite<Suzy::Sprite::NORMAL>::colliding( 0x3 ),
    SuzySprite<Suzy::Sprite::NORMAL>::colliding( 0x4 ),
    SuzySprite<Suzy::Sprite::NORMAL>::colliding( 0x5 ),
    SuzySprite<Suzy::Sprite::NORMAL>::colliding( 0x6 ),
    SuzySprite<Suzy::Sprite::NORMAL>::colliding( 0x7 ),
    SuzySprite<Suzy::Sprite::NORMAL>::colliding( 0x8 ),
    SuzySprite<Suzy::Sprite::NORMAL>::colliding( 0x9 ),
    SuzySprite<Suzy::Sprite::NORMAL>::colliding( 0xa ),
    SuzySprite<Suzy::Sprite::NORMAL>::colliding( 0xb ),
    SuzySprite<Suzy::Sprite::NORMAL>::colliding( 0xc ),
    SuzySprite<Suzy::Sprite::NORMAL>::colliding( 0xd ),
    SuzySprite<Suzy::Sprite::NORMAL>::colliding( 0xe ),
    SuzySprite<Suzy::Sprite::NORMAL>::colliding( 0xf )
  },
  std::array<bool, ColOperator::POSSIBLE_PIXELS>
  {
    SuzySprite<Suzy::Sprite::NONCOLL>::colliding( 0x0 ),
    SuzySprite<Suzy::Sprite::NONCOLL>::colliding( 0x1 ),
    SuzySprite<Suzy::Sprite::NONCOLL>::colliding( 0x2 ),
    SuzySprite<Suzy::Sprite::NONCOLL>::colliding( 0x3 ),
    SuzySprite<Suzy::Sprite::NONCOLL>::colliding( 0x4 ),
    SuzySprite<Suzy::Sprite::NONCOLL>::colliding( 0x5 ),
    SuzySprite<Suzy::Sprite::NONCOLL>::colliding( 0x6 ),
    SuzySprite<Suzy::Sprite::NONCOLL>::colliding( 0x7 ),
    SuzySprite<Suzy::Sprite::NONCOLL>::colliding( 0x8 ),
    SuzySprite<Suzy::Sprite::NONCOLL>::colliding( 0x9 ),
    SuzySprite<Suzy::Sprite::NONCOLL>::colliding( 0xa ),
    SuzySprite<Suzy::Sprite::NONCOLL>::colliding( 0xb ),
    SuzySprite<Suzy::Sprite::NONCOLL>::colliding( 0xc ),
    SuzySprite<Suzy::Sprite::NONCOLL>::colliding( 0xd ),
    SuzySprite<Suzy::Sprite::NONCOLL>::colliding( 0xe ),
    SuzySprite<Suzy::Sprite::NONCOLL>::colliding( 0xf )
  },
  std::array<bool, ColOperator::POSSIBLE_PIXELS>
  {
    SuzySprite<Suzy::Sprite::XOR>::colliding( 0x0 ),
    SuzySprite<Suzy::Sprite::XOR>::colliding( 0x1 ),
    SuzySprite<Suzy::Sprite::XOR>::colliding( 0x2 ),
    SuzySprite<Suzy::Sprite::XOR>::colliding( 0x3 ),
    SuzySprite<Suzy::Sprite::XOR>::colliding( 0x4 ),
    SuzySprite<Suzy::Sprite::XOR>::colliding( 0x5 ),
    SuzySprite<Suzy::Sprite::XOR>::colliding( 0x6 ),
    SuzySprite<Suzy::Sprite::XOR>::colliding( 0x7 ),
    SuzySprite<Suzy::Sprite::XOR>::colliding( 0x8 ),
    SuzySprite<Suzy::Sprite::XOR>::colliding( 0x9 ),
    SuzySprite<Suzy::Sprite::XOR>::colliding( 0xa ),
    SuzySprite<Suzy::Sprite::XOR>::colliding( 0xb ),
    SuzySprite<Suzy::Sprite::XOR>::colliding( 0xc ),
    SuzySprite<Suzy::Sprite::XOR>::colliding( 0xd ),
    SuzySprite<Suzy::Sprite::XOR>::colliding( 0xe ),
    SuzySprite<Suzy::Sprite::XOR>::colliding( 0xf )
  },
  std::array<bool, ColOperator::POSSIBLE_PIXELS>
  {
    SuzySprite<Suzy::Sprite::SHADOW>::colliding( 0x0 ),
    SuzySprite<Suzy::Sprite::SHADOW>::colliding( 0x1 ),
    SuzySprite<Suzy::Sprite::SHADOW>::colliding( 0x2 ),
    SuzySprite<Suzy::Sprite::SHADOW>::colliding( 0x3 ),
    SuzySprite<Suzy::Sprite::SHADOW>::colliding( 0x4 ),
    SuzySprite<Suzy::Sprite::SHADOW>::colliding( 0x5 ),
    SuzySprite<Suzy::Sprite::SHADOW>::colliding( 0x6 ),
    SuzySprite<Suzy::Sprite::SHADOW>::colliding( 0x7 ),
    SuzySprite<Suzy::Sprite::SHADOW>::colliding( 0x8 ),
    SuzySprite<Suzy::Sprite::SHADOW>::colliding( 0x9 ),
    SuzySprite<Suzy::Sprite::SHADOW>::colliding( 0xa ),
    SuzySprite<Suzy::Sprite::SHADOW>::colliding( 0xb ),
    SuzySprite<Suzy::Sprite::SHADOW>::colliding( 0xc ),
    SuzySprite<Suzy::Sprite::SHADOW>::colliding( 0xd ),
    SuzySprite<Suzy::Sprite::SHADOW>::colliding( 0xe ),
    SuzySprite<Suzy::Sprite::SHADOW>::colliding( 0xf )
  }
};

//whether collision depository is updatable for each sprite type
static constexpr std::array<bool, ColOperator::SPRITE_TYPES> depositoryUpdatable
{
  SuzySprite<Suzy::Sprite::BACKGROUND>::collDep,
  SuzySprite<Suzy::Sprite::BACKNONCOLL>::collDep,
  SuzySprite<Suzy::Sprite::BSHADOW>::collDep,
  SuzySprite<Suzy::Sprite::BOUNDARY>::collDep,
  SuzySprite<Suzy::Sprite::NORMAL>::collDep,
  SuzySprite<Suzy::Sprite::NONCOLL>::collDep,
  SuzySprite<Suzy::Sprite::XOR>::collDep,
  SuzySprite<Suzy::Sprite::SHADOW>::collDep
};

}


ColOperator::ColOperator( Suzy::Sprite spriteType, uint8_t sprColl ) : mSpriteType{ (size_t)spriteType }, mCollidingColors{ statesTables[mSpriteType] }, mMask{}, mHiColl{}, mStoreOff{}, mColAdr{}, mColl{ (uint16_t)( ( sprColl & 0xf ) | ( ( sprColl & 0xf ) << 4 ) ) }
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
  int32_t hposrem = hpos & 7;

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

  if ( mCollidingColors[pixel] )
  {
    if constexpr ( std::endian::native == std::endian::little )
    {
      mMask |= 0x0000000f << ( ( hposrem ^ 1 ) * 4 );
    }
    else
    {
      mMask |= 0xf0000000 >> ( hposrem * 4 );
    }
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

    return t76543210;
  }
  else
    return {};
}

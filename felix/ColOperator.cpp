#include "ColOperator.hpp"
#include "SpriteTemplates.hpp"

template<typename Type>
bool preprocessFun( bool edge, int hposLast, int hposCurrent )
{
  if constexpr ( Type::collDep )
  {
    if ( edge )
      return true;
    else
    {
      return ( hposLast >> 3 ) != ( hposCurrent >> 3 );
    }
  }
  else
  {
    return false;
  }
}

template<typename Type, int PIXEL>
bool processFun()
{
  return Type::colliding( PIXEL ) && Type::collWrite;
}

template<int... I>
static std::array<bool, ColOperator::STATES_SIZE> makeProcessStates( std::integer_sequence<int, I...> )
{
  return { processFun<SuzySprite<( Suzy::Sprite )( I / ColOperator::POSSIBLE_PIXELS )>,(I&(ColOperator::POSSIBLE_PIXELS-1))>()... };
}

template<int... I>
static std::array<bool, 8> makeSpriteCollideable( std::integer_sequence<int, I...> )
{
  return { SuzySprite<( Suzy::Sprite )I>::collWrite... };
}

template<int... I>
static std::array<bool, 8> makeDepositoryUpdatable( std::integer_sequence<int, I...> )
{
  return { SuzySprite<( Suzy::Sprite )I>::collDep... };
}


ColOperator::ColOperator( Suzy::Sprite spriteType, uint8_t sprColl ) :
  mProcesStates{ makeProcessStates( std::make_integer_sequence<int, STATES_SIZE>{} ) },
  mSpriteCollideable{ makeSpriteCollideable( std::make_integer_sequence<int,8>{} ) },
  mDepositoryUpdatable{ makeDepositoryUpdatable( std::make_integer_sequence<int, 8>{} ) },
  mSpriteType{ (int)spriteType * ColOperator::POSSIBLE_PIXELS },
  mMask{}, mStoreAddr{}, mColAdr{}, mColl{ (uint8_t)( ( sprColl & 0xf ) | ( ( sprColl & 0xf ) << 4 ) ) }, mHiColl{}
{
}

ColOperator::MemOp ColOperator::flush()
{
  if ( mMask )
  {
    return MemOp{ mMask, ( uint16_t )( mColAdr + mStoreAddr ), mColl };
  }
  else
  {
    return {};
  }
}

void ColOperator::newLine( uint16_t coladr )
{
  mColAdr = coladr;
  mStoreAddr = 0xffff;  //invalid address to detect first access
  mMask = 0x0000;
}

ColOperator::MemOp ColOperator::process( int hpos, uint8_t pixel )
{
  MemOp result{};

  uint16_t hposfloor = (uint16_t)( hpos & ~7 );
  int hposrem = hpos & 7;

  if ( mStoreAddr != hposfloor )
  {
    if ( mMask )
    {
      result = MemOp{ mMask, (uint16_t)( mColAdr + mStoreAddr ), mColl };
      mMask = 0;
    }
    mStoreAddr = hposfloor;
  }

  assert( pixel < 16 );
  if ( mProcesStates[mSpriteType * ColOperator::POSSIBLE_PIXELS + pixel] )
  {
    mMask |= 0xf0000000 >> ( hposrem * 4 );
  }
 
  return result;
}

void ColOperator::receiveHiColl( uint8_t value )
{
  if ( mDepositoryUpdatable[mSpriteType] )
  {
    mHiColl = std::max( mHiColl, value );
  }
}

uint8_t ColOperator::hiColl() const
{
  return mHiColl;
}

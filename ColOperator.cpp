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

template<typename Type>
ColOperator::ProcessResult processFun( ColOperator::ProcessArg arg )
{
  //struct ProcessArg
  //{
  //  uint8_t inColl;
  //  uint8_t sprColl;
  //  uint8_t hiColl;
  //  uint8_t pixel;
  //};

  //struct ProcessResult
  //{
  //  uint8_t hiCol;
  //  uint8_t outCol;
  //  uint8_t outMask;
  //};

  if ( Type::colliding( arg.pixel ) )
  {
    if constexpr ( Type::collWrite )
    {
      if constexpr ( Type::collDep )
      {
        return { std::max( arg.inColl, arg.hiColl), arg.sprColl, 0xf };
      }
      else
      {
        return { arg.hiColl, arg.sprColl, 0xf };
      }
    }
    else
    {
      if constexpr ( Type::collDep )
      {
        return { std::max( arg.inColl, arg.hiColl ), 0x0, 0x0 };
      }
      else
      {
        return { arg.hiColl, 0x0, 0x0 };
      }
    }
  }
  else
  {
    return { arg.hiColl, 0x0, 0x0 };
  }
}

template<int... I>
static std::array<ColOperator::preprocessFunT, 8> makePreprocessFunc( std::integer_sequence<int, I...> )
{
  return { preprocessFun<SuzySprite<( Suzy::Sprite )I>>... };
}

template<int... I>
static std::array<ColOperator::processFunT, 8> makeProcessFunc( std::integer_sequence<int, I...> )
{
  return { processFun<SuzySprite<( Suzy::Sprite )I>>... };
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
  mPreprocesFuncs{ makePreprocessFunc( std::make_integer_sequence<int, 8>{} ) },
  mProcesFuncs{ makeProcessFunc( std::make_integer_sequence<int, 8>{} ) },
  mSpriteCollideable{ makeSpriteCollideable( std::make_integer_sequence<int,8>{} ) },
  mDepositoryUpdatable{ makeDepositoryUpdatable( std::make_integer_sequence<int, 8>{} ) },
  mSpriteType{ (int)spriteType },
  mColl{ (uint8_t)( sprColl & Suzy::SPRCOLL::NUMBER_MASK ) },
  mEnabled{ ( sprColl & Suzy::SPRCOLL::NO_COLLIDE ) != 0 }
{
}

ColOperator::MemOp ColOperator::flush()
{
  if ( !mEnabled )
    return MemOp{};

  return MemOp{};
}

void ColOperator::newLine( uint16_t vidadr )
{
}

ColOperator::MemOp ColOperator::preProcess( int hpos )
{
  if ( !mEnabled )
    return MemOp{};

  return MemOp{};
}

ColOperator::MemOp ColOperator::process( int hpos, uint8_t pixel )
{
  if ( !mEnabled )
    return MemOp{};

  return MemOp{};
}

void ColOperator::read( uint16_t value )
{
}

bool ColOperator::enabled() const
{
  return mEnabled;
}

uint8_t ColOperator::hiColl() const
{
  return mHiColl;
}

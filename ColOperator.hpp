#pragma once
#include "Suzy.hpp"
#include <array>

enum class Suzy::Sprite;

class ColOperator
{
public:
  struct MemOp
  {
    static constexpr uint8_t NONE   = 0;
    static constexpr uint8_t READ   = 1;
    static constexpr uint8_t WRITE  = 2;

    uint32_t value;
    uint16_t addr;
    uint16_t op;

    operator uint16_t() const
    {
      return op;
    }
  };

  static_assert( sizeof( MemOp ) == sizeof( uint64_t ) );

public:
  ColOperator( Suzy::Sprite spriteType, uint8_t sprColl );

  MemOp flush();
  void newLine( uint16_t vidadr );

  MemOp preProcess( int hpos );
  MemOp process( int hpos, uint8_t pixel );
  void read( uint16_t value );
  bool enabled() const;
  uint8_t hiColl() const;

  struct ProcessArg
  {
    uint8_t inColl;
    uint8_t sprColl;
    uint8_t hiColl;
    uint8_t pixel;
  };

  struct ProcessResult
  {
    uint8_t hiCol;
    uint8_t outCol;
    uint8_t outMask;
  };


  typedef bool ( *preprocessFunT )( bool edge, int hposLast, int hposCurrent );
  typedef ProcessResult( *processFunT )( ProcessArg );

private:
  std::array<preprocessFunT, 8> mPreprocesFuncs;
  std::array<processFunT, 8> mProcesFuncs;
  std::array<bool, 8> mSpriteCollideable;
  std::array<bool, 8> mDepositoryUpdatable;
  int mSpriteType;
  uint32_t mStore;
  int mOff;
  uint16_t mColAdr;
  uint8_t mEdge;
  uint8_t mColl;
  uint8_t mHiColl;
  bool mEnabled;

};

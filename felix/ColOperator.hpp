#pragma once
#include "Suzy.hpp"

enum class Suzy::Sprite;

class ColOperator
{
public:
  struct MemOp
  {
    union
    {
      struct
      {
        uint32_t mask;
        uint16_t addr;
        uint8_t value;
      };
      uint64_t word;
    };

    explicit operator bool() const
    {
      return word != 0;
    }
  };

  static_assert( sizeof( MemOp ) == sizeof( uint64_t ) );

public:
  ColOperator( Suzy::Sprite spriteType, uint8_t sprColl );

  MemOp flush();
  void newLine( uint16_t coladr );

  MemOp process( int hpos, uint8_t pixel );
  void receiveHiColl( uint8_t value );
  uint8_t hiColl() const;

  typedef bool( *processFunT )( uint8_t );

  static constexpr size_t POSSIBLE_PIXELS = 16;
  static constexpr size_t SPRITE_TYPES = 8;

  static constexpr size_t STATES_SIZE = POSSIBLE_PIXELS * SPRITE_TYPES;

private:
  //processing functions for each sprite type
  std::array<bool, STATES_SIZE> mProcesStates;
  //whether each sprite type is collideable
  std::array<bool, 8> mSpriteCollideable;
  //whether collision depository is updatable for each sprite type
  std::array<bool, 8> mDepositoryUpdatable;
  //sprite type
  size_t mSpriteType;
  //mask of updated nibbles of collision buffer
  uint32_t mMask;
  //address of current 8-pixel collision buffer cache. It's initialized to impossible value
  uint16_t mStoreAddr;
  //address of current collision buffer line
  uint16_t mColAdr;
  //sprite's collision number
  uint8_t mColl;
  //highest collistion number detected
  uint8_t mHiColl;
};

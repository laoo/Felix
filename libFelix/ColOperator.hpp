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
        uint16_t value;
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
  void receiveHiColl( uint32_t value );
  std::optional<uint8_t> hiColl() const;

  typedef bool( *processFunT )( uint8_t );

  static constexpr size_t POSSIBLE_PIXELS = 16;
  static constexpr size_t SPRITE_TYPES = 8;

  static constexpr size_t STATES_SIZE = POSSIBLE_PIXELS * SPRITE_TYPES;

private:
  //sprite type
  size_t mSpriteType;
  //mask of updated nibbles of collision buffer
  uint32_t mMask;
  //highest collistion number detected
  uint32_t mHiColl;
  //offset from the beginning of line of current 8-pixel collision buffer cache. It's initialized to impossible value
  int32_t mStoreOff;
  //address of current collision buffer line
  uint16_t mColAdr;
  //quadrupled sprite's collision number
  uint16_t mColl;
};

#pragma once
#include <cstdint>
#include <array>
class SuzyMath
{
public:
  SuzyMath();

  bool poke( uint64_t tick, uint8_t offset, uint8_t value );
  void wpoke( uint64_t tick, uint8_t offset, uint16_t value );
  uint8_t peek( uint64_t tick, uint8_t offset );
  void mul( uint64_t tick );
  void div( uint64_t tick );
  void signAB();
  void signCD();

  bool signMath() const;
  bool accumulate() const;
  bool working() const;
  bool warning() const;
  bool carry() const;
  bool unsafeAccess() const;
  void signMath( bool value );
  void accumulate( bool value );
  void carry( bool value );
  void unsafeAccess( bool value );

private:

  uint32_t abcd() const;
  uint32_t efgh() const;
  uint32_t jklm() const;
  uint16_t ab() const;
  uint16_t cd() const;
  uint16_t np() const;

  void abcd( uint32_t value );
  void efgh( uint32_t value );
  void jklm( uint32_t value );
  void ab( uint16_t value );
  void cd( uint16_t value );
  void np( uint16_t value );

  void mul();
  void div();

private:

  alignas( 8 ) std::array<uint8_t, 32> mArea;

  uint64_t mFinishTick;
  int mSignAB;
  int mSignCD;
  enum class Op
  {
    NONE,
    MULTIPLY,
    DIVIDE
  } mOp;
  bool mUnsafeAccess;
  bool mSignMath;           //Signmath: 0 = unsigned math, 1 = signed math.
  bool mAccumulate;         //OK to accumvlate : 0 = do not accumulate, 1 = yes, accumulate.
  bool mMathWarning;       //Mathbit: If mult, 1 = accumulator overflow.If div, 1 = div by zero attempted.
  bool mMathCarry;         //Last carry bit.

};

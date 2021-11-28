#include "pch.hpp"
#include "SuzyMath.hpp"
#include "TraceHelper.hpp"
#include "Utility.hpp"

namespace
{
static constexpr size_t off_abcd = 0x52 - 0x50;
static constexpr size_t off_ab = 0x54 - 0x50;
static constexpr size_t off_cd = 0x52 - 0x50;
static constexpr size_t off_np = 0x56 - 0x50;
static constexpr size_t off_efgh = 0x60 - 0x50;
static constexpr size_t off_jklm = 0x6c - 0x50;

uint16_t convertSigned( uint16_t value, int & sign )
{
  // "In signed multiply, the hardware thinks that 8000 is a positive number."

  // "In signed multiply, the hardware thinks that 0 is a negative number. This is not an 
  // immediate problem for a multiply by zero, since the answer will be re-negated to the 
  // correct polarity of zero. However, since it will set the sign flag, you can not depend 
  // on the sign flag to be correct if you just load the lower byte after a multiply by zero."

  // Do conversion if value is negative. Subtract 1 to account for 0 being negative
  sign = 1;
  if ( ( ( value - 1 ) & 0x8000 ) == 0x8000 )
  {
    uint16_t conversion = ( uint16_t )( value ^ 0xFFFF );
    conversion++; // Add 1 for earlier correction
    sign = -1;
    value = conversion;
  }
  return value;
}

}

SuzyMath::SuzyMath( std::shared_ptr<TraceHelper> traceHelper ) : mArea{}, mTraceHelper{ std::move( traceHelper ) }, mFinishTick{}, mSignAB{}, mSignCD{}, mUnsafeAccess{}, mSignMath{}, mAccumulate{}, mMathWarning{}, mMathCarry{}
{
  std::fill( mArea.begin(), mArea.end(), 0xff );
}

bool SuzyMath::poke( uint64_t tick, uint8_t offset, uint8_t value )
{
  assert( ( offset >= 0x50 ) && ( offset < 0x70 ) );

  if ( tick < mFinishTick )
  {
    mUnsafeAccess = true;
  }

  size_t index = offset - 0x50;

  mArea[index] = value;
  return true;
}

void SuzyMath::wpoke( uint64_t tick, uint8_t offset, uint16_t value )
{
  assert( ( offset >= 0x50 ) && ( offset < 0x70 ) && ( ( offset & 1 ) == 0 ) );

  if ( tick < mFinishTick )
  {
    mUnsafeAccess = true;  
  }

  size_t index = offset - 0x50;

  *((uint16_t*)( mArea.data() + index ) ) = value;
}

uint8_t SuzyMath::peek( uint64_t tick, uint8_t offset )
{
  assert( ( offset >= 0x50 ) && ( offset < 0x70 ) );

  if ( tick < mFinishTick )
  {
    mUnsafeAccess = true;
  }

  size_t index = offset - 0x50;
  return mArea[index];
}

void SuzyMath::mul( uint64_t tick )
{
  mFinishTick = tick + ( ( mSignMath || mAccumulate ) ? 54 : 44 );

  mMathWarning = false;

  uint32_t valueAB = ab();
  uint32_t valueCD = cd();
  uint32_t result = valueAB * valueCD;

  if ( mSignMath )
  {
    int signEFGH = mSignAB + mSignCD; // Add the sign bits. Zero means negative result	
    if ( signEFGH == 0 )
    {
      result ^= 0xffffffff; // Calculate 2-s complement
      result++;
    }
  }

  efgh( result );

  if ( mAccumulate )
  {
    uint32_t valueJKLM = jklm();
    uint32_t acc = valueJKLM + result;
    mMathWarning = mMathCarry = ( acc & 0x80000000 ) != ( valueJKLM & 0x80000000 );
    jklm( acc );

    mTraceHelper->comment( "math: {} {:04x}*{:04x}={:08x}, acc = {:08x}", ( mSignMath ? 's' : 'u' ), valueAB, valueCD, result, (uint32_t)acc);

    if ( mMathCarry )
      mTraceHelper->comment( "carry" );
  }
  else
  {
    mTraceHelper->comment( "math: {:04x}*{:04x}={:08x}", valueAB, valueCD, result );
  }
}

void SuzyMath::div( uint64_t tick )
{
  // "Divides take 176 + 14*N ticks where N is the number of most significant zeros in the divisor."
  uint64_t n = std::countl_zero( np() );
  mFinishTick = tick + 176 + 14 * n;

  mMathWarning = false;

  uint32_t valueNP = np();
  uint32_t valueEFGH = efgh();

  if ( valueNP != 0 )
  {
    uint32_t valueABCD = valueEFGH / valueNP;
    uint32_t valueJKLM = valueEFGH % valueNP;

    abcd( valueABCD );
    jklm( valueJKLM );

    if ( valueJKLM )
    {
      mTraceHelper->comment( "math: {0:08x}/{1:04x}={2:04x} + {3:08x}/{1:04x}", valueEFGH, valueNP, valueABCD, valueJKLM );
    }
    else
    {
      mTraceHelper->comment( "math: {:08x}/{:04x}={:04x}", valueEFGH, valueNP, valueABCD );
    }
  }
  else
  {
    abcd( 0xffffffff );
    jklm( 0 );

    mMathWarning = mMathCarry = true;
  }
}

void SuzyMath::signAB()
{
  if ( mSignMath )
  {
     uint16_t tmp = convertSigned( ab(), mSignAB );
     ab( tmp );
  }
}

void SuzyMath::signCD()
{
  if ( mSignMath )
  {
    uint16_t tmp = convertSigned( cd(), mSignCD );
    cd( tmp );
  }
}

bool SuzyMath::signMath() const
{
  return mSignMath;
}

bool SuzyMath::accumulate() const
{
  return mAccumulate;
}

bool SuzyMath::working( uint64_t tick ) const
{
  return tick < mFinishTick;
}

bool SuzyMath::warning() const
{
  return mMathWarning;
}

bool SuzyMath::carry() const
{
  return mMathCarry;
}

bool SuzyMath::unsafeAccess() const
{
  return mUnsafeAccess;
}

void SuzyMath::signMath( bool value )
{
  mSignMath = value;
}

void SuzyMath::accumulate( bool value )
{
  mAccumulate = value;
}

void SuzyMath::carry( bool value )
{
  mMathCarry = value;
}

void SuzyMath::unsafeAccess( bool value )
{
  mUnsafeAccess = value;
}

uint32_t SuzyMath::abcd() const
{
  return *( ( uint32_t* )( mArea.data() + off_abcd ) );
}

uint32_t SuzyMath::efgh() const
{
  return *( ( uint32_t* )( mArea.data() + off_efgh ) );
}

uint32_t SuzyMath::jklm() const
{
  return *( ( uint32_t* )( mArea.data() + off_jklm ) );
}

uint16_t SuzyMath::ab() const
{
  return *( ( uint16_t* )( mArea.data() + off_ab ) );
}

uint16_t SuzyMath::cd() const
{
  return *( ( uint16_t* )( mArea.data() + off_cd ) );
}

uint16_t SuzyMath::np() const
{
  return *( ( uint16_t* )( mArea.data() + off_np ) );
}

void SuzyMath::abcd( uint32_t value )
{
  *( ( uint32_t* )( mArea.data() + off_abcd ) ) = value;
}

void SuzyMath::efgh( uint32_t value )
{
  *( ( uint32_t* )( mArea.data() + off_efgh ) ) = value;
}

void SuzyMath::jklm( uint32_t value )
{
  *( ( uint32_t* )( mArea.data() + off_jklm ) ) = value;
}

void SuzyMath::ab( uint16_t value )
{
  *( ( uint16_t* )( mArea.data() + off_ab ) ) = value;
}

void SuzyMath::cd( uint16_t value )
{
  *( ( uint16_t* )( mArea.data() + off_cd ) ) = value;
}

void SuzyMath::np( uint16_t value )
{
  *( ( uint16_t* )( mArea.data() + off_np ) ) = value;
}



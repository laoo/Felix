#include "SuzyCoroutines.hpp"

namespace
{
//https://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64Bits
inline uint8_t reverse( uint8_t b )
{
  return (uint8_t)( ( ( b * 0x80200802ULL ) & 0x0884422110ULL ) * 0x0101010101ULL >> 32 );
}
}


PixelUnpacker::PixelUnpacker( handle c ) : mCoro{ c },
  mShifter{}, mSize{}, mTotalSize{},
  mBPP{}, mTotallyLiteral{}
{
  mCoro.promise().unpacker = this;
}

PixelUnpacker::PixelUnpacker( PixelUnpacker && other ) noexcept : mCoro{ std::move( other.mCoro ) },
  mShifter{ other.mShifter }, mSize{ other.mSize }, mTotalSize{ other.mTotalSize },
  mBPP{ other.mBPP }, mTotallyLiteral{ other.mTotallyLiteral }
{
  other.mCoro = nullptr;
}

PixelUnpacker & PixelUnpacker::operator=( PixelUnpacker && other ) noexcept
{
  std::swap( *this, other );
  return *this;
}

PixelUnpacker::~PixelUnpacker()
{
  if ( mCoro )
    mCoro.destroy();
}

uint8_t PixelUnpacker::startLine( uint32_t bpp, bool totallyLiteral, uint32_t initialData )
{
  mBPP = bpp;
  mTotallyLiteral = totallyLiteral;
  populateShifter( initialData );
#ifndef NDEBUG
  mTotalSize = 8; //to not trigger assertion in pull
#endif
  uint8_t sprdoff = pull<8>();

  mTotalSize = ( sprdoff - 1 ) * 8;

  mCoro();

  return uint8_t();
}

void PixelUnpacker::populateShifter( uint32_t data )
{
  for ( int i = 0; i < 4; ++i )
  {
    assert( mSize <= 56 );
    assert( ( mShifter & ( ( 1ull << mSize ) - 1 ) ) == 0 );
    uint64_t b = reverse( ( data >> ( i * 8 )) & 0xff );
    mShifter |= ( b << mSize );
    mSize += 8;
  }
}

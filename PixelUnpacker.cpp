#include "PixelUnpacker.hpp"

namespace
{
//https://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64Bits
inline uint8_t reverse( uint8_t b )
{
  return ( uint8_t )( ( ( b * 0x80200802ULL ) & 0x0884422110ULL ) * 0x0101010101ULL >> 32 );
}
}

PixelUnpacker::PixelUnpacker( handle c ) : mCoro{ c },
  mShifter{}, mSize{}, mTotalSize{}, mSprDoff{},
  mBPP{}, mLiteral{},
  mLine{}, mResult{}
{
  mCoro.promise().unpacker = this;
}

PixelUnpacker::PixelUnpacker( PixelUnpacker && other ) noexcept : mCoro{ std::move( other.mCoro ) },
  mShifter{ other.mShifter }, mSize{ other.mSize }, mTotalSize{ other.mTotalSize }, mSprDoff{ other.mSprDoff },
  mBPP{ other.mBPP }, mLiteral{ other.mLiteral },
  mLine{ other.mLine }, mResult{}
{
  other.mCoro = nullptr;
}

PixelUnpacker::~PixelUnpacker()
{
  if ( mCoro )
    mCoro.destroy();
}


PixelUnpacker::Result PixelUnpacker::operator()()
{
  mCoro();
  return mResult;
}

int PixelUnpacker::pull( uint32_t bits )
{
  uint64_t result{};
  for ( uint32_t i = 0; i < bits; ++i )
  {
    result = ( result << 1 ) | ( mShifter & 1 );
    mShifter >>= 1;
  }
  mSize -= bits;
  mTotalSize -= bits;
  assert( mSize >= 0 );
  assert( mTotalSize >= 0 );
  return ( int )result;
}


void PixelUnpacker::feedData( uint32_t data )
{
  for ( int i = 0; i < 4; ++i )
  {
    assert( mSize <= 56 );
    assert( ( mShifter & ~( ( 1ull << mSize ) - 1 ) ) == 0 );
    uint64_t b = reverse( ( data >> ( i * 8 ) ) & 0xff );
    mShifter |= ( b << mSize );
    mSize += 8;
  }
}

uint8_t PixelUnpacker::startLine( int32_t bpp, bool totallyLiteral, uint32_t initialData )
{
  mShifter = 0;
  mBPP = bpp;
  mLiteral = totallyLiteral;
  feedData( initialData );
  mTotalSize = 8;
  mSprDoff = pull<8>();

  return (uint8_t)mSprDoff;
}

bool PixelUnpacker::nextLine()
{
  if ( mSprDoff == 0 )
  {
    setResult( { Status::END_OF_SPRITE } );
    return false;
  }
  else if ( mSprDoff == 1 )
  {
    setResult( { Status::END_OF_QUADRANT } );
    return false;
  }
  else if ( mSize == 0 )
  {
    setResult( { Status::END_OF_LINE } );
    return false;
  }
  else
  {
    return true;
  }
}

PixelUnpacker::Line * PixelUnpacker::getLine()
{
  if ( mSprDoff == 0 )
  {
    return nullptr;
  }

  mTotalSize = ( mSprDoff - 1 ) * 8;
  return &mLine;
}

bool PixelUnpacker::nextPen()
{
  if ( !mLine.literal && mLine.count > 0 )
    return true;

  int32_t reqBits =  mBPP + ( mLiteral || ( mLine.literal && mLine.count > 0 ) ? 0 : 5 );

  if ( mTotalSize <= reqBits )
  {
    mSize = 0;
    return true;
  }

  if ( mSize < reqBits )
  {
    setResult( { Status::DATA_NEEDED } );
    return false;
  }

  return true;
}

uint8_t * PixelUnpacker::getPen()
{
  if ( mLiteral )
  {
    if ( mTotalSize > mBPP )
    {
      mLine.pen = pull( mBPP );
      return &mLine.pen;
    }
    else
    {
      return nullptr;
    }
  }
  else
  {
    if ( mLine.count > 0 )
    {
      mLine.count -= 1;
      if ( mLine.literal )
      {
        mLine.pen = pull( mBPP );
      }
      return &mLine.pen;
    }

    mLine.literal = pull<1>();
    mLine.count = pull<4>();

    if ( mLine.literal || mLine.count > 0 )
    {
      mLine.pen = pull( mBPP );
      return &mLine.pen;
    }

    return nullptr;
  }
}


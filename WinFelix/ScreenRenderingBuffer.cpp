#include "pch.hpp"
#include "ScreenRenderingBuffer.hpp"

ScreenRenderingBuffer::ScreenRenderingBuffer() : mCurrentRow{}
{
  std::ranges::fill( mSizes, 0 );
  newRow( 104 );
}

void ScreenRenderingBuffer::newRow( int row )
{
  int idx = 104 - std::clamp( row, 0, 104 );
  mCurrentRow = &mRows[idx];
  mSize = &mSizes[idx];
  *mSize = 0;
}

ScreenRenderingBuffer::Row const& ScreenRenderingBuffer::row( size_t i ) const
{
  assert( i < ROWS_COUNT );
  return mRows[i];
}

int ScreenRenderingBuffer::size( size_t i ) const
{
  assert( i < ROWS_COUNT );
  return mSizes[i];
}

void ScreenRenderingBuffer::pushScreenBytes( std::span<uint8_t const> data )
{
  int idx = *mSize;
  for ( uint8_t byte : data )
  {
    mCurrentRow->at( idx++ ) = 0xff00 | (uint16_t)byte;
  }
  *mSize = idx & ( LINE_BUFFER_SIZE - 1 );
}

void ScreenRenderingBuffer::pushColorChage( uint8_t reg, uint8_t value )
{
  int idx = *mSize;
  mCurrentRow->at(idx++) = ( reg << 8 ) | value;

  *mSize = idx & ( LINE_BUFFER_SIZE - 1 );
}

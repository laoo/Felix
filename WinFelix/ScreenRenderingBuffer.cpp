#include "ScreenRenderingBuffer.hpp"

ScreenRenderingBuffer::ScreenRenderingBuffer() : mSizes{}, mRows{}, mCurrentRow{}, mSize{}
{
  std::ranges::fill( mSizes, 0 );
  std::fill_n( mRows.data()->data(), ( sizeof mRows ) / sizeof( uint16_t), 0 );
  std::fill_n( mCurrentRow.data(), ( sizeof mCurrentRow ) / sizeof(uint16_t), 0);
}

void ScreenRenderingBuffer::newRow( int row )
{
  std::copy_n( mCurrentRow.data(), mSize, mRows[( row + 1 ) % ROWS_COUNT].data() );
  mSizes[( row + 1 ) % ROWS_COUNT] = mSize;
  mSize = 0;
}

std::span<uint16_t const> ScreenRenderingBuffer::row( size_t i ) const
{
  assert( i < ROWS_COUNT );
  size_t idx = ROWS_COUNT - i - 1;
  return std::span<uint16_t const>{ mRows[idx].data(), (size_t)mSizes[idx] };
}

void ScreenRenderingBuffer::pushScreenBytes( std::span<uint8_t const> data )
{
  int idx = mSize;
  for ( uint8_t byte : data )
  {
    mCurrentRow[idx++] = 0xff00 | ( uint16_t )byte;
  }
  mSize = idx & ( LINE_BUFFER_SIZE - 1 );
}

void ScreenRenderingBuffer::pushColorChage( uint8_t reg, uint8_t value )
{
  int idx = mSize;
  mCurrentRow[idx++] = ( reg << 8 ) | value;
  mSize = idx & ( LINE_BUFFER_SIZE - 1 );
}

#pragma once

class ScreenRenderingBuffer
{
public:
  static constexpr size_t DISPLAY_BYTES = 80;
  static constexpr size_t MAX_COLOR_CHANGES = 256;
  static constexpr size_t LINE_BUFFER_SIZE = 512;
  static constexpr size_t ROWS_COUNT = 105;
  static_assert( DISPLAY_BYTES + MAX_COLOR_CHANGES <= LINE_BUFFER_SIZE );

  using Row = std::array<uint16_t, LINE_BUFFER_SIZE>;

  ScreenRenderingBuffer();

  void newRow( int row );

  void pushColorChage( uint8_t reg, uint8_t value );
  void pushScreenBytes( std::span<uint8_t const> data );

  std::span<uint16_t const> row( size_t i ) const;

private:
  std::array<int, ROWS_COUNT> mSizes;
  std::array<Row, ROWS_COUNT> mRows;
  Row mCurrentRow;
  int mSize;
};


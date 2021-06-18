#pragma once

class DisplayLine
{
public:
  DisplayLine( int32_t displayRow ) : mIndex{}, mDisplayRow{ displayRow }
  {
  }

  void addByte( uint8_t value )
  {
    mBuffer[mIndex++] = DisplayByte{ value };
  }

  void addColorChange( uint8_t value, uint8_t reg )
  {
    mBuffer[mIndex++] = ColorChange{ value, reg };
  }

private:

  static constexpr size_t DISPLAY_BYTES = 80;
  static constexpr size_t MAX_COLOR_CHANGES = 256;
  static constexpr size_t BUFFER_SIZE = DISPLAY_BYTES + MAX_COLOR_CHANGES;

  struct DisplayByte
  {
    uint8_t value;
  };

  struct ColorChange
  {
    uint8_t value;
    uint8_t reg;
  };

  std::array<std::variant<DisplayByte, ColorChange>, BUFFER_SIZE> mBuffer;
  uint32_t mIndex;
  int32_t mDisplayRow;
};

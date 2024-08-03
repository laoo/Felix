#pragma once

#include <filesystem>
#include <vector>
#include <span>
#include <array>

class SpriteDumper
{
  struct Desc
  {
    uint32_t crc;
    uint32_t id;
    uint8_t minx;
    uint8_t miny;
    uint8_t maxx;
    uint8_t maxy;

    int width() const
    {
      return maxx - minx + 1;
    }

    int height() const
    {
      return maxy - miny + 1;
    }
  };

  std::filesystem::path mOutputPath;
  std::vector<Desc> mProcessedSprites = {};
  std::vector<uint8_t> mData = {};
  uint16_t mWindowAddress = {};
  Desc mCurrectDesc = {};
  std::array<uint32_t, 16> mPalette;
  std::array<uint32_t, 160 * 102> mScreen = {};
  bool mNewSprite = true;
  bool mBackground = false;

public:
  SpriteDumper( std::filesystem::path outputPath );
  void setPalette( std::span<uint8_t const> palette );

  void startSprite( uint16_t windowAddress, int16_t posx, int16_t posy, bool background );
  uint8_t fetch( uint8_t value );
  uint32_t fetch( uint32_t value );
  void drawByte( uint16_t address, uint8_t value, uint8_t mask );

private:
  void createPNG( std::filesystem::path outputPath );
  std::pair<uint8_t, uint8_t> pixelPos( uint32_t off ) const;
};


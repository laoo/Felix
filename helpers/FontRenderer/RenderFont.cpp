#include <cstdint>
#include <fstream>
#include <vector>
#include <filesystem>
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

std::vector<uint8_t> readFont( std::filesystem::path fontPath )
{
  size_t size = std::filesystem::file_size( fontPath );
  std::vector<uint8_t> fontBuffer;
  fontBuffer.resize( size );
  std::ifstream fin{ fontPath, std::ios::binary };
  fin.read( (char*)fontBuffer.data(), size );
  return fontBuffer;
}

std::vector<uint8_t> renderFont( std::filesystem::path fontPath, int boxWidth, int boxHeight, float fontHeight )
{
  std::vector<uint8_t> fontBuffer = readFont( fontPath );
  if ( fontBuffer.empty() )
    return {};

  int imageWidth = boxWidth;
  int imageHeight = 256 * boxHeight;

  std::vector<uint8_t> result;
  result.resize( imageWidth * imageHeight );

  stbtt_fontinfo font;
  stbtt_InitFont( &font, fontBuffer.data(), 0 );

  float scale = stbtt_ScaleForPixelHeight( &font, fontHeight );
  int ascent;
  stbtt_GetFontVMetrics( &font, &ascent, nullptr, nullptr );
  int baseline = (int)( ascent * scale );

  for ( int i = 0; i < 256; ++i )
  {
    if ( stbtt_FindGlyphIndex( &font, i ) )
    {
      int x0, y0, x1, y1;
      stbtt_GetCodepointBitmapBox( &font, i, scale, scale, &x0, &y0, &x1, &y1 );
      stbtt_MakeCodepointBitmap( &font, result.data() + ( baseline + y0 + i * boxHeight ) * imageWidth, x1 - x0, y1 - y0, imageWidth, scale, scale, i );
    }
  }

  return result;
}

std::vector<uint8_t> renderFont( std::vector<uint8_t> bitmap, int boxHeight )
{
  static constexpr int imageWidth = 8;
  int imageHeight = 256 * boxHeight;

  std::vector<uint8_t> result;
  result.reserve( imageWidth * imageHeight );

  for ( int i = 0; i < imageHeight; ++i )
  {
    for ( int k = 0; k < imageWidth; ++k )
    {
      result.push_back( bitmap[i] & ( 1 << ( 7 - k ) ) ? 0xff : 00 );
    }
  }

  return result;
}


int main( int argc, char** argv )
{
  auto data = readFont( "..\\..\\libextern\\vga-text-mode-fonts\\FONTS\\BIGPILE\\SWISSBX2.F16" );
  data = renderFont( std::move( data ), 16 );

  std::ofstream fout{ "..\\..\\WinFelix\\fonts.hpp" };

  fout << "//Automatically generated from \"..\\libextern\\vga-text-mode-fonts\\FONTS\\BIGPILE\\SWISSBX2.F16\"\n\n";
  fout << "uint8_t font_SWISSBX2[] = {\n";
  for ( size_t i = 0; i < data.size(); i += 8 )
  {
    auto limit = std::min( i + 8, data.size() );
    fout << "\t";
    for ( size_t j = i; j < limit; ++j )
    {
      fout << "0x" << std::hex << std::setfill( '0' ) << std::setw( 2 ) << (int)data[j];
      if ( j + 1 != data.size() )
        fout << ',';
    }
    fout << "\n";
  }
  fout << "};\n";
}

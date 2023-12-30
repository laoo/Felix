#include "Utility.hpp"

std::vector<uint8_t> readFile( std::filesystem::path const& path )
{
  if ( !std::filesystem::exists( path ) )
    return {};

  size_t size = (size_t)std::filesystem::file_size( path );
  if ( size == 0 )
    return {};

  std::vector<uint8_t> data( size, 0 );

  std::ifstream fin{ path, std::ios::binary };
  fin.read( (char*)data.data(), size );

  return data;
}

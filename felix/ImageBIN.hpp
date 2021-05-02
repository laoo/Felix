#pragma once

class ImageBIN
{
public:

public:
  ImageBIN( std::vector<uint8_t> data );

  void load( uint8_t * memory ) const;

private:
  std::vector<uint8_t> const mData;
};

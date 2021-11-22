#pragma once

class ImageBIOS
{
public:

public:
  ImageBIOS( std::vector<uint8_t> data );

  void load( std::span<uint8_t> memory ) const;

private:
  std::vector<uint8_t> const mData;
};

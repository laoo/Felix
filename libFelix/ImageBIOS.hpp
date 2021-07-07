#pragma once

struct MemU;

class ImageBIOS
{
public:

public:
  ImageBIOS( std::vector<uint8_t> data );

  void load( std::span<MemU> memory ) const;

private:
  std::vector<uint8_t> const mData;
};

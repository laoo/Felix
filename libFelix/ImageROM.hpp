  #pragma once

class ImageROM
{
public:

public:
  static std::shared_ptr<ImageROM const> create( std::filesystem::path const& path );

  ImageROM( std::vector<uint8_t> data );

  void load( std::span<uint8_t> memory ) const;

private:
  std::vector<uint8_t> const mData;
};

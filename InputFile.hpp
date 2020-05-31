#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

class ImageBS93;

class InputFile
{
public:
  enum class FileType
  {
    UNKNOWN,
    BS93
  };

  InputFile( std::filesystem::path const& path );

  bool valid() const;
  FileType getType() const;

  std::shared_ptr<ImageBS93> getBS93() const;
private:
  std::shared_ptr<ImageBS93> checkBS93( std::vector<uint8_t> && data ) const;

private:
  FileType mType;
  std::shared_ptr<ImageBS93> mBS93;

  ;
};

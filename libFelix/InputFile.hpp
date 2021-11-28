#pragma once

class ImageBS93;
class ImageCart;
class ImageProperties;

class InputFile
{
public:
  enum class FileType
  {
    UNKNOWN,
    BS93,
    CART,
  };

  InputFile( std::filesystem::path const& path, ImageProperties & imageProperties );

  bool valid() const;
  FileType getType() const;

  std::shared_ptr<ImageBS93 const> getBS93() const;
  std::shared_ptr<ImageCart const> getCart() const;

private:
  FileType mType;
  std::shared_ptr<ImageBS93 const> mBS93;
  std::shared_ptr<ImageCart const> mCart;
};

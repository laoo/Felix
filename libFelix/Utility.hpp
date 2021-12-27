#pragma once

class NonCopyable
{
public:
  NonCopyable( NonCopyable const& ) = delete;
  NonCopyable& operator=( NonCopyable const& ) = delete;

protected:
  NonCopyable() = default;
  ~NonCopyable() = default;
};

struct AudioSample
{
  int16_t left;
  int16_t right;
};

enum class RunMode
{
  PAUSE,
  STEP,
  RUN
};

std::vector<uint8_t> readFile( std::filesystem::path const& path );


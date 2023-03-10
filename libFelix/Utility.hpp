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

enum class CpuBreakType
{
  //No cpu break
  NONE,
  //CPU break on next instruction boundary on batch end
  NEXT,
  //CPU break on next instruction boundary in a response to RunMode::STEP_IN
  STEP_IN,
  //CPU break if CPU did not go into a subroutine in a response to RunMode::STEP_OVER
  STEP_OVER,
  //CPU break if CPU goes out from a subroutine in a response to RunMode::STEP_OUT
  STEP_OUT,
  //CPU break on brk instruction
  BRK_INSTRUCTION,
  //trap break
  TRAP_BREAK
};

enum class RunMode
{
  PAUSE,
  STEP_IN,
  STEP_OVER,
  STEP_OUT,
  RUN
};

std::vector<uint8_t> readFile( std::filesystem::path const& path );

static constexpr int SCREEN_WIDTH = 160;
static constexpr int SCREEN_HEIGHT = 102;

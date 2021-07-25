#include "pch.hpp"
#include "GameDrive.hpp"

GameDrive::GameDrive( std::filesystem::path const& imagePath ) : mBasePath{ imagePath.parent_path() }, mBuffer{}, mGDCoroutine{ process() }, mHasOutput{}, mRunning{}
{
}

GameDrive::~GameDrive()
{
}

bool GameDrive::hasOutput( uint64_t tick ) const
{
  return mHasOutput;
}

bool GameDrive::running() const
{
  return mRunning;
}

void GameDrive::put( uint8_t value )
{
  if ( !mHasOutput )
  {
    mBuffer.value = value;
    mGDCoroutine.resume();
  }
}

uint8_t GameDrive::get( uint64_t tick )
{
  if ( mHasOutput )
  {
    mGDCoroutine.resume();
    return mBuffer.value;
  }

  return 0;
}

GameDrive::GDCoroutine GameDrive::process()
{
  std::filesystem::path base = mBasePath;
  std::filesystem::path path{};
  std::fstream file{};

  for ( ;; )
  {
    switch ( (ECommandByte)co_await getCommand() )
    {
    case ECommandByte::OpenDir:
      co_yield FRESULT::NOT_ENABLED;
      break;
    case ECommandByte::ReadDir:
      co_yield FRESULT::NOT_ENABLED;
      break;
    case ECommandByte::OpenFile:
    {
      if ( file.is_open() )
      {
        co_yield FRESULT::NOT_READY;
        break;
      }
      std::string fname{};
      for ( ;; )
      {
        uint8_t b = co_await getByte();
        if ( b == 0 )
          break;
        fname += (char)b;
      }
      path = base / fname;
      if ( std::filesystem::exists( path ) )
      {
        file.open( path, std::ios::binary | std::ios::in );
        co_yield FRESULT::OK;
      }
      else
      {
        co_yield FRESULT::NO_FILE;
      }
      break;
    }
    case ECommandByte::GetSize:
      co_yield FRESULT::NOT_ENABLED;
      break;
    case ECommandByte::Seek:
      co_yield FRESULT::NOT_ENABLED;
      break;
    case ECommandByte::Read:
    {
      if ( !file.is_open() )
      {
        co_yield FRESULT::NOT_OPENED;
        break;
      }
      int32_t size = co_await getByte();
      size |= ( co_await getByte() ) << 8;

      while ( size-- >= 0 )
      {
        co_yield (uint8_t)file.get();
      }
      co_yield FRESULT::OK;
      break;
    }
    case ECommandByte::Write:
      co_yield FRESULT::NOT_ENABLED;
      break;
    case ECommandByte::Close:
      co_yield FRESULT::NOT_ENABLED;
      break;
    case ECommandByte::ProgramFile:
      co_yield FRESULT::NOT_ENABLED;
      break;
    case ECommandByte::ClearBlocks:
      co_yield FRESULT::NOT_ENABLED;
      break;
    case ECommandByte::LowPowerMode:
      co_yield FRESULT::NOT_ENABLED;
      break;
    default:
      co_yield FRESULT::NOT_ENABLED;
      break;
    }
  }
}


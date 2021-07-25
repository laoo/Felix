#include "pch.hpp"
#include "GameDrive.hpp"

GameDrive::GameDrive( std::filesystem::path const& imagePath ) : mBasePath{ imagePath.parent_path() }, mBuffer{}, mGDCoroutine{ process() }, mReadTick{}, mRunning{}
{
}

GameDrive::~GameDrive()
{
}

bool GameDrive::hasOutput( uint64_t tick ) const
{
  return mReadTick.has_value() && *mReadTick < tick;
}

bool GameDrive::running() const
{
  return mRunning;
}

void GameDrive::put( uint64_t tick, uint8_t value )
{
  if ( !mReadTick.has_value() )
  {
    mLastTick = tick;
    mBuffer.value = value;
    mGDCoroutine.resume();
  }
}

uint8_t GameDrive::get( uint64_t tick )
{
  if ( mReadTick.has_value() )
  {
    mLastTick = tick;
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
  size_t bytesRead{};
  static constexpr uint64_t byteReadLatency = 234;
  static constexpr uint64_t blockReadLatency = 163 * 47 * 16;

  for ( ;; )
  {
    switch ( (ECommandByte)co_await getByte() )
    {
    case ECommandByte::OpenDir:
      co_await putResult( FRESULT::NOT_ENABLED );
      break;
    case ECommandByte::ReadDir:
      co_await putResult( FRESULT::NOT_ENABLED );
      break;
    case ECommandByte::OpenFile:
    {
      if ( file.is_open() )
      {
        co_await putResult( FRESULT::NOT_READY );
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
        co_await putResult( FRESULT::OK );
        bytesRead = 0;
      }
      else
      {
        co_await putResult( FRESULT::NO_FILE );
      }
      break;
    }
    case ECommandByte::GetSize:
      co_await putResult( FRESULT::NOT_ENABLED );
      break;
    case ECommandByte::Seek:
      co_await putResult( FRESULT::NOT_ENABLED );
      break;
    case ECommandByte::Read:
    {
      if ( !file.is_open() )
      {
        co_await putResult( FRESULT::NOT_OPENED );
        break;
      }
      int32_t size = co_await getByte();
      size |= ( co_await getByte() ) << 8;

      while ( size-- >= 0 )
      {
        uint64_t latency = ( ( bytesRead++ ) & 0x7fff ) == 0 ? blockReadLatency : byteReadLatency;
        co_await putByte( (uint8_t)file.get(), latency );
      }
      co_await putResult( FRESULT::OK );
      break;
    }
    case ECommandByte::Write:
      co_await putResult( FRESULT::NOT_ENABLED );
      break;
    case ECommandByte::Close:
      co_await putResult( FRESULT::NOT_ENABLED );
      break;
    case ECommandByte::ProgramFile:
      co_await putResult( FRESULT::NOT_ENABLED );
      break;
    case ECommandByte::ClearBlocks:
      co_await putResult( FRESULT::NOT_ENABLED );
      break;
    case ECommandByte::LowPowerMode:
      co_await putResult( FRESULT::NOT_ENABLED );
      break;
    default:
      co_await putResult( FRESULT::NOT_ENABLED );
      break;
    }
  }
}


#include "pch.hpp"
#include "GameDrive.hpp"
#include "CartBank.hpp"
#include "Log.hpp"

GameDrive::GameDrive( std::filesystem::path const& imagePath ) : mMemoryBank{}, mBasePath { imagePath.parent_path() }, mBuffer{}, mGDCoroutine{ process() }, mReadTick{}
{
  mBaseTime = std::chrono::steady_clock::now();
  mLastTimePoint = 0;
}

GameDrive::~GameDrive()
{
}

bool GameDrive::hasOutput( uint64_t tick ) const
{
  return mReadTick.has_value() && *mReadTick < tick;
}

void GameDrive::put( uint64_t tick, uint8_t value )
{
  if ( mBuffer.ready )
  {
    mLastTick = tick;
    mBuffer.ready = false;
    mBuffer.value = value;
    mGDCoroutine.resume();
  }
}

uint8_t GameDrive::get( uint64_t tick )
{
  mLastTick = tick;
  mReadTick = std::nullopt;
  auto result = mBuffer.value;
  mGDCoroutine.resume();
  return result;
}

CartBank* GameDrive::getBank( uint64_t tick ) const
{
  return mProgrammedBank.get();
}

GameDrive::GDCoroutine GameDrive::process()
{
  std::filesystem::path base = mBasePath;
  std::filesystem::path path{};
  std::fstream file{};
  size_t bytesRead{};
  static constexpr uint64_t byteReadLatency = 120;
  static constexpr uint64_t blockReadLatency = 159 * 5 * 16;
  static constexpr uint64_t programByteLatency = 34;

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
        file.close();
      }
      std::string fname{};
      for ( ;; )
      {
        uint8_t b = co_await getByte();
        if ( b == 0 )
          break;
        if ( b == '/' && fname.empty() )
          continue;
        fname += (char)b;
      }
      path = base / fname;
      if ( std::filesystem::exists( path ) )
      {
        file.open( path, std::ios::binary | std::ios::in );
        co_await putResult( file.good() ? FRESULT::OK : FRESULT::NOT_OPENED );
        bytesRead = 0;
      }
      else
      {
        co_await putResult( FRESULT::NO_FILE );
      }
      break;
    }
    case ECommandByte::GetSize:
    {
      uint32_t size = file.is_open() ? (uint32_t)std::filesystem::file_size( path ) : 0;
      co_await putByte( (uint8_t)( ( size >> 0 ) & 0xff ) );
      co_await putByte( (uint8_t)( ( size >> 8 ) & 0xff ) );
      co_await putByte( (uint8_t)( ( size >> 16 ) & 0xff ) );
      co_await putByte( (uint8_t)( ( size >> 24 ) & 0xff ) );
    }
      break;
    case ECommandByte::Seek:
    {
      uint32_t offset = co_await getByte();
      offset |= ( co_await getByte() ) << 8;
      offset |= ( co_await getByte() ) << 16;
      offset |= ( co_await getByte() ) << 24;
      if ( !file.is_open() )
      {
        co_await putResult( FRESULT::NOT_OPENED );
      }
      else
      {
        file.seekg( offset );
        co_await putResult( FRESULT::OK );
      }
    }
      break;
    case ECommandByte::Read:
    {
      int32_t size = co_await getByte();
      size |= ( co_await getByte() ) << 8;

      if ( !file.is_open() )
      {
        while ( size-- > 0 )
        {
          co_await putByte( 0 );
        }
        co_await putResult( FRESULT::NOT_OPENED );
      }
      else
      {
        while ( size-- > 0 )
        {
          uint64_t latency = ( ( bytesRead++ ) & 0x7fff ) == 0 ? blockReadLatency : byteReadLatency;
          uint8_t b = file.get();
          co_await putByte( b, latency );
        }
        co_await putResult( FRESULT::OK );
      }
      break;
    }
    case ECommandByte::Write:
      co_await putResult( FRESULT::NOT_ENABLED );
      break;
    case ECommandByte::Close:
      if ( !file.is_open() )
      {
        co_await putResult( FRESULT::NOT_OPENED );
      }
      else
      {
        file.close();
        co_await putResult( FRESULT::OK );
      }
      break;
    case ECommandByte::ProgramFile:
    {
      size_t startBlock = co_await getByte();
      co_await getByte(); //unused high byte of start block
      size_t blockSize = 256 * co_await getByte();
      size_t blockCount = co_await getByte();
      blockCount |= (size_t)( co_await getByte() ) << 8; //unused hight byte of block count
      if ( !file.is_open() )
      {
        co_await putResult( FRESULT::NOT_OPENED );
      }
      else
      {
        blockCount = std::max( blockCount, 256ull );
        for ( size_t i = 0; i < blockCount; ++i )
        {
          file.read( std::bit_cast<char*>( mMemoryBank.data() ) + 2048 * ( startBlock + i ), blockSize );
        }
        mProgrammedBank = std::make_shared<CartBank>( std::span<uint8_t const>{ mMemoryBank.data(), mMemoryBank.size() } );
        {
          auto now = std::chrono::steady_clock::now();
          auto diff = std::chrono::duration_cast<std::chrono::milliseconds>( now - mBaseTime );
          double timePoint = (double)diff.count() / 1000.0;
          L_DEBUG << "start program: " << ( timePoint - mLastTimePoint );
          mLastTimePoint = timePoint;
        }
        co_await putResult( FRESULT::OK, blockCount * blockSize * programByteLatency );
        {
          auto now = std::chrono::steady_clock::now();
          auto diff = std::chrono::duration_cast<std::chrono::milliseconds>( now - mBaseTime );
          double timePoint = (double)diff.count() / 1000.0;
          L_DEBUG << "end program: " << ( timePoint - mLastTimePoint );
          mLastTimePoint = timePoint;
        }
      }
    }
      break;
    case ECommandByte::ClearBlocks:
    {
      size_t startBlock = co_await getByte();
      co_await getByte(); //unused high byte of start block
      size_t blockCount = co_await getByte();
      co_await getByte(); //unused hight byte of block count
      for ( size_t i = 0; i < blockCount; ++i )
      {
        std::fill_n( mMemoryBank.data() + 2048 * ( startBlock + i ), 2048, 0 );
      }
      co_await putResult( FRESULT::OK );
    }
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


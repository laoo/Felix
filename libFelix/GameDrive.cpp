#include "pch.hpp"
#include "GameDrive.hpp"
#include "CartBank.hpp"
#include "ImageCart.hpp"
#include "Log.hpp"

GameDrive::GameDrive( std::filesystem::path const& imagePath ) : mMemoryBank{}, mBasePath { imagePath.parent_path() }, mBuffer{}, mGDCoroutine{ process() }, mReadTick{}
{
  mBaseTime = std::chrono::steady_clock::now();
  mLastTimePoint = 0;
}

GameDrive::~GameDrive()
{
}

std::unique_ptr<GameDrive> GameDrive::create( ImageCart const& cart )
{
  if ( cart.eeprom().sd() )
  {
    return std::make_unique<GameDrive>( cart.path() );
  }
  else
  {
    return {};
  }
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
  std::vector<uint8_t> finData{};
  size_t fileOffset{};
  static constexpr uint64_t byteReadLatency = 120;
  static constexpr uint64_t blockReadLatency = 159 * 5 * 16;
  static constexpr uint64_t programByteLatency = 34;

  auto readByte = [&]()
  {
    if ( fileOffset < finData.size() )
    {
      return finData[fileOffset++];
    }
    else
    {
      return uint8_t{};
    }
  };

  for ( ;; )
  {
    auto cmd = (ECommandByte)co_await getByte();
    switch ( cmd )
    {
    case ECommandByte::OpenDir:
      L_DEBUG << "GD OpenDir NYI";
      co_await putResult( FRESULT::NOT_ENABLED );
      break;
    case ECommandByte::ReadDir:
      L_DEBUG << "GD ReadDir NYI";
      co_await putResult( FRESULT::NOT_ENABLED );
      break;
    case ECommandByte::OpenFile:
    {
      if ( file.is_open() )
      {
        file.close();
      }
      finData.clear();
      fileOffset = 0;
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
        L_DEBUG << "GD Open file " << path;
        file.open( path, std::ios::binary | std::ios::in );
        uint32_t size = (uint32_t)std::filesystem::file_size( path );
        finData.resize( size );
        file.read( (char*)finData.data(), finData.size() );
        file.close();
        co_await putResult( file.good() ? FRESULT::OK : FRESULT::NOT_OPENED );
      }
      else
      {
        L_DEBUG << "GD File " << path << " open error";
        co_await putResult( FRESULT::NO_FILE );
      }
      break;
    }
    case ECommandByte::GetSize:
    {
      uint32_t size =  file.is_open() ? (uint32_t)std::filesystem::file_size( path ) : (uint32_t)finData.size();
      L_DEBUG << "GD File size " << size;
      co_await putByte( (uint8_t)( ( size >> 0 ) & 0xff ) );
      co_await putByte( (uint8_t)( ( size >> 8 ) & 0xff ) );
      co_await putByte( (uint8_t)( ( size >> 16 ) & 0xff ) );
      co_await putByte( (uint8_t)( ( size >> 24 ) & 0xff ) );
      break;
    }
    case ECommandByte::Seek:
    {
      uint32_t offset = co_await getByte();
      offset |= ( co_await getByte() ) << 8;
      offset |= ( co_await getByte() ) << 16;
      offset |= ( co_await getByte() ) << 24;
      if ( finData.empty() )
      {
        L_DEBUG << "GD File seek not opened";
        co_await putResult( FRESULT::NOT_OPENED );
      }
      else
      {
        if ( offset > finData.size() )
        {
          L_DEBUG << "GD File resized from " << finData.size() << " to " << offset << std::hex << "($" << offset << ")";
          finData.resize( offset );
        }
        L_DEBUG << "GD File seek " << offset;
        fileOffset = offset;
        co_await putResult( FRESULT::OK );
      }
      break;
    }
    case ECommandByte::Read:
    {
      int32_t size = co_await getByte();
      size |= ( co_await getByte() ) << 8;

      if ( fileOffset < finData.size() && fileOffset + size <= finData.size() )
      {
        L_DEBUG << "GD Read " << size << "\t[" << fileOffset << "," << fileOffset + size << ")\t\t$" << std::hex << size << "\t[$" << fileOffset << ",$" << fileOffset + size << ")";
      }
      else if ( fileOffset < finData.size() && fileOffset + size > finData.size() )
      {
        L_DEBUG << "GD Read " << size << "\t[" << fileOffset << "," << finData.size() << ") | " << fileOffset + size - finData.size() << " * 0\t\t$" << std::hex << size << "\t[$" << fileOffset << ",$" << finData.size() << ") | $" << fileOffset + size - finData.size() << " * 0";
      }
      else
      {
        L_DEBUG << "GD Read " << size << "\t" << size << " * 0\t\t$" << std::hex << size << "\t$" << size << " * 0";
      }

      while ( size-- > 0 )
      {
        co_await putByte( readByte(), byteReadLatency );
      }
      co_await putResult( finData.empty() ? FRESULT::NOT_OPENED : FRESULT::OK );
      break;
    }
    case ECommandByte::Write:
      L_DEBUG << "GD Write NYI";
      co_await putResult( FRESULT::NOT_ENABLED );
      break;
    case ECommandByte::Close:
      if ( finData.empty() )
      {
        L_DEBUG << "GD Close not opened";
        co_await putResult( FRESULT::NOT_OPENED );
      }
      else
      {
        file.close();
        finData.clear();
        fileOffset = 0;
        L_DEBUG << "GD Close";
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
      if ( finData.empty() )
      {
        L_DEBUG << "GD Program not opened";
        co_await putResult( FRESULT::NOT_OPENED );
      }
      else
      {
        blockCount = std::max( blockCount, 256ull );
        size_t size = blockCount * blockSize;

        if ( fileOffset < finData.size() && fileOffset + size <= finData.size() )
        {
          L_DEBUG << "GD Program " << size << "\t[" << fileOffset << "," << fileOffset + size << ") to start:" << startBlock << ", blockSize:" << blockSize << ", blockCount:" << blockCount << "\t\t$" << size << "\t[$" << fileOffset << ",$" << fileOffset + size << ") to start:$" << startBlock << ", blockSize:$" << blockSize << ", blockCount:$" << blockCount;
        }
        else if ( fileOffset < finData.size() && fileOffset + size > finData.size() )
        {
          L_DEBUG << "GD Read " << size << "\t[" << fileOffset << "," << finData.size() << ") | " << fileOffset + size - finData.size() << " * 0 to start:" << startBlock << ", blockSize:" << blockSize << ", blockCount:" << blockCount << "\t\t$" << size << "\t[$" << fileOffset << ",$" << finData.size() << ") | $" << fileOffset + size - finData.size() << " * 0 to start:$" << startBlock << ", blockSize:$" << blockSize << ", blockCount:$" << blockCount;
        }
        else
        {
          L_DEBUG << "GD Read " << size << "\t" << size << " * 0 to start:" << startBlock << ", blockSize:" << blockSize << ", blockCount:" << blockCount << "\t\t$" << size << "\t$" << size << " * 0 to start:$" << startBlock << ", blockSize:$" << blockSize << ", blockCount:$" << blockCount;
        }

        for ( size_t i = 0; i < blockCount; ++i )
        {
          for ( size_t j = 0; j < blockSize; ++j )
          {
            mMemoryBank[2048 * ( startBlock + i ) + j] = readByte();
          }
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
      break;
    }
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
      L_DEBUG << "GD Clear start:" << startBlock << ", blockCount:" << blockCount;

      co_await putResult( FRESULT::OK );
      break;
    }
    case ECommandByte::LowPowerMode:
      L_DEBUG << "GD LowPowerMode NYI";
      co_await putResult( FRESULT::NOT_ENABLED );
      break;
    default:
      L_DEBUG << "GD Unknown command " << (int)cmd;
      co_await putResult( FRESULT::NOT_ENABLED );
      break;
    }
  }
}


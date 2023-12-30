#include "VGMWriter.hpp"

VGMWriter::VGMWriter( std::filesystem::path path ) : mHeader{}, mFout { path, std::ios::binary }, mLastTick{}
{
  mFout.seekp( sizeof( VGMHeader ) );
}

VGMWriter::~VGMWriter()
{
  mFout.put( CMD_END_OF_SOUND_DATA );
  uint64_t size = mFout.tellp();

  mHeader.EofOffset = (uint32_t)( size - offsetof( VGMHeader, EofOffset ) );
  mHeader.Total_samples = tickToSample( mLastTick );

  mFout.seekp( 0 );
  mFout.write( (char const*)&mHeader, sizeof( VGMHeader ) );
}

void VGMWriter::init( uint64_t tick )
{
  mLastTick = tick;
}

void VGMWriter::write( uint64_t tick, uint8_t reg, uint8_t val )
{
  auto lastSample = tickToSample( mLastTick );
  auto currentSample = tickToSample( tick );
  auto samplesDiff = currentSample - lastSample;

  while ( samplesDiff > 0 )
  {
    if ( samplesDiff <= CMD_SHORT_WAIT_MAX )
    {
      mFout.put( CMD_SHORT_WAIT + samplesDiff - 1 );
      samplesDiff = 0;
    }
    else
    {
      uint16_t wait = (uint16_t)std::min( CMD_LONG_WAIT_MAX, samplesDiff );
      mFout.put( CMD_LONG_WAIT );
      mFout.put( wait & 0xff );
      mFout.put( wait >> 8 );
      samplesDiff -= wait;
    }
  }

  mFout.put( CMD_MIKEY );
  mFout.put( std::bit_cast<char>( reg ) );
  mFout.put( std::bit_cast<char>( val ) );

  mLastTick = tick;
}

uint32_t VGMWriter::tickToSample( uint64_t tick ) const
{
  return (uint32_t)( tick * SAMPLE_RATE / MIKEY_CLOCK );
}

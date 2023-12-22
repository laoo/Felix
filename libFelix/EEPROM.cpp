#include "pch.hpp"
#include "EEPROM.hpp"
#include "ImageProperties.hpp"
#include "TraceHelper.hpp"

EEPROM::EEPROM( std::filesystem::path imagePath, int eeType, bool is16Bit, std::shared_ptr<TraceHelper> traceHelper ) : mEECoroutine{}, mImagePath{ std::move( imagePath ) },
  mTraceHelper{ std::move( traceHelper ) }, mData{}, mOpcodeBits{}, mAddressMask{}, mDataBits{}, mWriteEnable{}, mChanged{ true }
{
  assert( eeType > 0 && eeType < 6 );

  switch ( eeType & 0x7 )
  {
  case 1: //93C46
    mOpcodeBits = 9;
    mAddressMask = 0x7F;
    break;
  case 2: //93C56
    mOpcodeBits = 11;
    mAddressMask = 0xFF;
    break;
  case 3: // 93C66
    mOpcodeBits = 11;
    mAddressMask = 0x1FF;
    break;
  case 4: // 93C76
    mOpcodeBits = 13;
    mAddressMask = 0x3FF;
    break;
  case 5: // 93C86
    mOpcodeBits = 13;
    mAddressMask = 0x7FF;
    break;
  default:
    break;
  }

  mData.resize( mAddressMask + 1 );
  std::ranges::fill( mData, 0xff );

  if ( is16Bit )
  {
    mOpcodeBits -= 1;
    mAddressMask >>= 1;
    mDataBits = 16;
  }
  else
  {
    mDataBits = 8;
  }

  if ( std::filesystem::exists( mImagePath ) )
  {
    auto size = std::min( (size_t)std::filesystem::file_size( mImagePath ), mData.size() );
    std::ifstream fin{ mImagePath, std::ios::binary };
    fin.read( (char*)mData.data(), size );
    mChanged = false;
  }
}

EEPROM::~EEPROM()
{
  if ( mChanged )
  {
    try
    {
      std::ofstream fout{ mImagePath, std::ios::binary };
      fout.write( (char const*)mData.data(), mData.size() );
    }
    catch ( ... )
    {
    }
  }
}

std::unique_ptr<EEPROM> EEPROM::create( ImageProperties const& imageProperties, std::shared_ptr<TraceHelper> traceHelper )
{
  auto ee = imageProperties.getEEPROM();

  if ( ee.type() > 0 && ee.type() < 6 )
  {
    auto path = imageProperties.getPath();
    path.replace_extension( path.extension().string() + ".e2p" );

    return std::make_unique<EEPROM>( std::move( path ), ee.type(), ee.is16Bit(), std::move( traceHelper ) );
  }
  else
  {
    return {};
  }
}

void EEPROM::tick( uint64_t tick, bool cs, bool audin )
{
  if ( io.cs )
  {
    if ( cs )
    {
      if ( io.busyUntil < tick )
      {
        io.currentTick = tick;
        io.input = audin;

        mEECoroutine();
      }
    }
    else
    {
      if ( (bool)mEECoroutine )
      {
        mTraceHelper->comment<"EEPROM: /CS.">( );
        mEECoroutine.reset();
      }
      io.cs = false;
    }
  }
  else if ( cs && audin )
  {
    if ( io.busyUntil < tick )
    {
      mTraceHelper->comment<"EEPROM: begin.">();
      mEECoroutine = process();
      io.output = {};
    }
    io.cs = true;
  }
}

std::optional<bool> EEPROM::output( uint64_t tick ) const
{
  if ( io.busyUntil < tick )
  {
    return io.output;
  }
  else
  {
    mTraceHelper->comment<"EEPROM: programming.">();
    return false;
  }
}

EEPROM::EECoroutine EEPROM::process()
{
  int opcode = 0;
  for ( int i = 0; i < mOpcodeBits; ++i )
  {
    opcode <<= 1;
    int bit = co_await io;
    opcode |= bit;
    mTraceHelper->comment<"EEPROM: fetch opcode bit {}={}.">( mOpcodeBits - i - 1, bit );
  }

  int cmd = opcode >> ( mOpcodeBits - 2 );
  int address = opcode & mAddressMask;
  int data = 0;
  int dataBits = mDataBits;

  switch ( cmd )
  {
  case 0b00:
    switch ( address >> ( mOpcodeBits - 4 ) )
    {
    case 0b00:  //EWDS
      ewds();
      break;
    case 0b01:  //WRAL
      for ( int i = 0; i < dataBits; ++i )
      {
        data <<= 1;
        int bit = co_await io;
        data |= bit;
      }
      wral( data );
      co_return true; 
    case 0b10: //ERAL
      eral();
      break;
    case 0b11: //EWEN
      ewen();
      break;
    }
    break;
  case 0b01: //WRITE
    for ( int i = 0; i < dataBits; ++i )
    {
      data <<= 1;
      int bit = co_await io;
      data |= bit;
      mTraceHelper->comment<"EEPROM: fetch data bit {}={}.">( dataBits - i - 1, bit );
    }
    write( address, data );
    co_return true;
  case 0b10: //READ
    co_yield 0;
    data = read( address );
    for ( int i = 0; i < dataBits; ++i )
    {
      int bit = ( data >> ( dataBits - i - 1 ) ) & 1;
      mTraceHelper->comment<"EEPROM: emit data bit {}={}.">( dataBits - i - 1, bit );
      co_yield bit;
    }
    break;
  case 0b11: //ERASE
    erase( address );
    co_return true;
  }

  co_return std::nullopt;
}

int EEPROM::read( int address ) const
{
  if ( mDataBits == 16 )
  {
    int data = 0xffff;
    address <<= 1;
    if ( address < ( int )mData.size() )
    {
      data = mData[address];
      data |= ( mData[address + 1] ) << 8;
    }
    mTraceHelper->comment<"EEPROM: EXECUTE READ16 ${:x} from ${:x}.">( data, address );
    return data;
  }
  else
  {
    int data = 0xff;
    if ( address < ( int )mData.size() )
    {
      data = mData[address];
    }
    mTraceHelper->comment<"EEPROM: EXECUTE READ8 ${:x} from ${:x}.">( data, address );
    return data;
  }
}

void EEPROM::ewen()
{
  mTraceHelper->comment<"EEPROM: EXECUTE EWEN.">();
  mWriteEnable = true;
}

void EEPROM::erase( int address )
{
  write( address, 0xffff, true );
}

void EEPROM::write( int address, int data, bool erase )
{
  if ( mWriteEnable )
  {
    if ( mDataBits == 16 )
    {
      address <<= 1;
      if ( address < ( int )mData.size() )
      {
        if ( mData[address] != ( data & 0xff ) )
        {
          mChanged = true;
          mData[address] = data & 0xff;
        }
        if ( mData[address + 1] != ( ( data >> 8 ) & 0xff ) )
        {
          mChanged = true;
          mData[address + 1] = ( data >> 8 ) & 0xff;
        }
      }
      if ( erase )
        mTraceHelper->comment< "EEPROM: EXECUTE ERASE16 ${:x}" >( address );
      else
        mTraceHelper->comment< "EEPROM: EXECUTE WRITE16 ${:x} to ${:x}." >( data, address );
    }
    else
    {
      if ( address < ( int )mData.size() )
      {
        mData[address] = data & 0xff;
      }
      if ( erase )
        mTraceHelper->comment< "EEPROM: EXECUTE ERASE8 ${:x}" >( address );
      else
        mTraceHelper->comment< "EEPROM: EXECUTE WRITE8 ${:x} to ${:x}." >( data, address );
    }

    startProgram( WRITE_TICKS );
  }
  else
  {
    if ( mDataBits == 16 )
    {
      address <<= 1;
      if ( erase )
        mTraceHelper->comment< "EEPROM: DISABLED ERASE16 ${:x}." >( address );
      else
        mTraceHelper->comment< "EEPROM: DISABLED WRITE16 ${:x} to ${:x}." >( data, address );
    }
    else
    {
      if ( erase )
        mTraceHelper->comment< "EEPROM: DISABLED ERASE8 ${:x}." >( address );
      else
        mTraceHelper->comment< "EEPROM: DISABLED WRITE8 ${:x} to ${:x}." >( data, address );
    }
  }
}

void EEPROM::eral()
{
  if ( mWriteEnable )
  {
    mTraceHelper->comment< "EEPROM: EXECUTE ERAL." >();
    std::ranges::fill( mData, 0xff );
    mChanged = true;
    startProgram( ERAL_TICKS );
  }
  else
  {
    mTraceHelper->comment< "EEPROM: DISABLED ERAL." >();
  }
}

void EEPROM::wral( int data )
{
  if ( mWriteEnable )
  {
    if ( mDataBits == 16 )
    {
      uint16_t* begin = (uint16_t*)mData.data();
      uint16_t* end = (uint16_t*)( mData.data() + mData.size() );

      std::fill( begin, end, data & 0xffff );
      mTraceHelper->comment< "EEPROM: EXECUTE WRAL16 ${:x}." >( data );
    }
    else
    {
      std::ranges::fill( mData, data & 0xff );
      mTraceHelper->comment< "EEPROM: EXECUTE WRAL8 ${:x}." >( data );
    }
    mChanged = true;
    startProgram( WRAL_TICKS );
  }
  else
  {
    if ( mDataBits == 16 )
    {
      mTraceHelper->comment< "EEPROM: DISABLED WRAL16 ${:x}." >( data );
    }
    else
    {
      mTraceHelper->comment< "EEPROM: DISABLED WRAL8 ${:x}." >( data );
    }
  }
}

void EEPROM::ewds()
{
  mTraceHelper->comment< "EEPROM: EXECUTE EWDS." >();
  mWriteEnable = false;
}

void EEPROM::startProgram( uint64_t duration )
{
  io.busyUntil = io.currentTick + WRITE_TICKS;
  io.output = true;
}
void EEPROM::EECoroutine::promise_type::return_value( std::optional<bool> opt )
{
  mEE.mTraceHelper->comment< "EEPROM: end." >();
  mEE.io.output = opt;
}

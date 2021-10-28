#include "pch.hpp"
#include "EEPROM.hpp"
#include "ImageCart.hpp"
#include "TraceHelper.hpp"

EEPROM::EEPROM( std::filesystem::path imagePath, int eeType, bool is16Bit, std::shared_ptr<TraceHelper> traceHelper ) : mEECoroutine{ process() }, mImagePath{ std::move( imagePath ) },
  mTraceHelper{ std::move( traceHelper ) }, mData{}, mAddressBits{}, mDataBits{}, mWriteEnable{}, mChanged{}
{
  assert( eeType != 0 );

  mAddressBits = 6 + eeType;
  mData.resize( 1ull << mAddressBits );
  std::fill( mData.begin(), mData.end(), 0xff );

  if ( is16Bit )
  {
    mAddressBits -= 1;
    mDataBits = 16;
  }
  else
  {
    mDataBits = 8;
  }

  if ( std::filesystem::exists( mImagePath ) )
  {
    auto size = std::min( std::filesystem::file_size( mImagePath ), mData.size() );
    std::ifstream fin{ mImagePath, std::ios::binary };
    fin.read( (char*)mData.data(), size );
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

std::unique_ptr<EEPROM> EEPROM::create( ImageCart const& cart, std::shared_ptr<TraceHelper> traceHelper )
{
  auto ee = cart.eeprom();

  if ( ee.type() != 0 )
  {
    auto path = cart.path();
    path.replace_extension( ".eeprom" );

    return std::make_unique<EEPROM>( std::move( path ), ee.type(), ee.is16Bit(), std::move( traceHelper ) );
  }
  else
  {
    return {};
  }
}

void EEPROM::tick( uint64_t tick, bool cs, bool audin )
{
  mIO.currentTick = tick;
  mIO.cs = cs;
  mIO.input = audin;
  if ( mIO.cs || mIO.started )
    mEECoroutine.resume();
}

std::optional<bool> EEPROM::output( uint64_t tick ) const
{
  if ( mIO.cs )
  {
    return mIO.busyUntil < mIO.currentTick ? mIO.output : std::optional<bool>{ false };
  }
  else
  {
    return {};
  }
}

EEPROM::EECoroutine EEPROM::process()
{
  for ( ;; )
  {
    try
    {
      if ( !co_await start() )
        continue;

      mTraceHelper->comment( "EEPROM: fetch start bit." );

      int cmd1 = co_await input();
      mTraceHelper->comment( "EEPROM: fetch cmd bit 1={}.", cmd1 );
      int cmd0 = co_await input();
      mTraceHelper->comment( "EEPROM: fetch cmd bit 0={}.", cmd0 );

      int cmd = ( cmd1 << 1 ) | cmd0;

      int address = 0;
      for ( int i = 0; i < mAddressBits; ++i )
      {
        address <<= 1;
        int bit = co_await input();
        address |= bit;
        mTraceHelper->comment( "EEPROM: fetch address bit {}={}.", mAddressBits - i - 1, bit );
      }

      int data = 0;

      switch ( cmd )
      {
      case 0b00:
        switch ( address >> ( mAddressBits - 2 ) )
        {
        case 0b00:  //EWDS
          ewds();
          break;
        case 0b01:  //WRAL
          for ( int i = 0; i < mDataBits; ++i )
          {
            data <<= 1;
            data |= co_await input();
          }
          wral( data );
          break;
        case 0b10: //ERAL
          eral();
          break;
        case 0b11: //EWEN
          ewen();
        }
        break;
      case 0b01: //WRITE
        for ( int i = 0; i < mDataBits; ++i )
        {
          data <<= 1;
          int bit = co_await input();
          data |= bit;
          mTraceHelper->comment( "EEPROM: fetch data bit {}={}.", mDataBits - i - 1, bit );
        }
        write( address, data );
        break;
      case 0b10: //READ
        co_yield 0;
        data = read( address );
        for ( int i = 0; i < mDataBits; ++i )
        {
          int bit = ( data >> ( mDataBits - i - 1 ) ) & 1;
          mTraceHelper->comment( "EEPROM: emit data bit {}={}.", mDataBits - i - 1, bit );
          co_yield bit;
        }
        break;
      case 0b11: //ERASE
        erase( address );
        break;
      }

      while ( !co_await finish() )
      {
        mTraceHelper->comment( "EEPROM: wait for /CS." );
        //repeat until cs is down
      }
    }
    catch ( [[maybe_unused]] NoCS const& )
    {
      mTraceHelper->comment( "EEPROM: unexpected /CS." );
      mIO.started = false;
    }
  }
}

int EEPROM::read( int address ) const
{
  if ( mDataBits == 16 )
  {
    int data = 0xffff;
    address <<= 1;
    if ( address < mData.size() )
    {
      data = mData[address];
      data |= ( mData[address + 1] ) << 8;
    }
    mTraceHelper->comment( "EEPROM: READ16 ${:x} from ${:x}.", data, address );
    return data;
  }
  else
  {
    int data = 0xff;
    if ( address < mData.size() )
    {
      data = mData[address];
    }
    mTraceHelper->comment( "EEPROM: READ8 ${:x} from ${:x}.", data, address );
    return data;
  }
}

void EEPROM::ewen()
{
  mTraceHelper->comment( "EEPROM: EWEN." );
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
      if ( address < mData.size() )
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
        mTraceHelper->comment( "EEPROM: ERASE16 ${:x}", address );
      else
        mTraceHelper->comment( "EEPROM: WRITE16 ${:x} to ${:x}.", data, address );
    }
    else
    {
      if ( address < mData.size() )
      {
        mData[address] = data & 0xff;
      }
      if ( erase )
        mTraceHelper->comment( "EEPROM: ERASE8 ${:x}", address );
      else
        mTraceHelper->comment( "EEPROM: WRITE8 ${:x} to ${:x}.", data, address );
    }

    mIO.busyUntil = mIO.currentTick + WRITE_TICKS;
  }
  else
  {
    if ( mDataBits == 16 )
    {
      address <<= 1;
      if ( erase )
        mTraceHelper->comment( "EEPROM: ERASE16 ${:x} DISABLED.", address );
      else
        mTraceHelper->comment( "EEPROM: WRITE16 ${:x} to ${:x} DISABLED.", data, address );
    }
    else
    {
      if ( erase )
        mTraceHelper->comment( "EEPROM: ERASE8 ${:x} DISABLED.", address );
      else
        mTraceHelper->comment( "EEPROM: WRITE8 ${:x} to ${:x} DISABLED.", data, address );
    }
  }
}

void EEPROM::eral()
{
  if ( mWriteEnable )
  {
    mTraceHelper->comment( "EEPROM: ERAL." );
    std::fill( mData.begin(), mData.end(), 0xff );
    mChanged = true;
    mIO.busyUntil = mIO.currentTick + ERAL_TICKS;
  }
  else
  {
    mTraceHelper->comment( "EEPROM: ERAL DISABLED." );
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
      mTraceHelper->comment( "EEPROM: WRAL16 ${:x}.", data );
    }
    else
    {
      std::fill( mData.begin(), mData.end(), data & 0xff );
      mTraceHelper->comment( "EEPROM: WRAL8 ${:x}.", data );
    }

    mChanged = true;
    mIO.busyUntil = mIO.currentTick + WRAL_TICKS;
  }
  else
  {
    if ( mDataBits == 16 )
    {
      mTraceHelper->comment( "EEPROM: WRAL16 ${:x} DISABLED.", data );
    }
    else
    {
      mTraceHelper->comment( "EEPROM: WRAL8 ${:x} DISABLED.", data );
    }
  }
}

void EEPROM::ewds()
{
  mTraceHelper->comment( "EEPROM: EWDS." );
  mWriteEnable = false;
}

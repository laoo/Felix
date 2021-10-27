#include "pch.hpp"
#include "EEPROM.hpp"
#include "ImageCart.hpp"

EEPROM::EEPROM( std::filesystem::path imagePath, int eeType, bool is16Bit ) : mEECoroutine{ process() }, mData{}, mAddressBits{}, mDataBits{}, mWriteEnable{}
{
  assert( eeType != 0 );

  mAddressBits = 6 + eeType;
  if ( is16Bit )
  {
    mAddressBits >>= 1;
    mDataBits = 16;
  }
  else
  {
    mDataBits = 8;
  }

  mData.resize( 1ull << mAddressBits );
}

EEPROM::~EEPROM()
{
}

std::unique_ptr<EEPROM> EEPROM::create( ImageCart const& cart )
{
  auto ee = cart.eeprom();

  if ( ee.type() != 0 )
  {
    auto path = cart.path();
    path.replace_extension( ".sav" );

    return std::make_unique<EEPROM>( std::move( path ), ee.type(), ee.is16Bit() );
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

      int cmd = co_await input() << 1;
      cmd |= co_await input();

      int address = 0;
      for ( int i = 0; i < mAddressBits; ++i )
      {
        address <<= 1;
        address |= co_await input();
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
          data |= co_await input();
        }
        write( address, data );
        break;
      case 0b10: //READ
        co_yield 0;
        data = read( address );
        for ( int i = 0; i < mDataBits; ++i )
        {
          co_yield data >> ( mDataBits - i - 1 );
        }
        break;
      case 0b11: //ERASE
        erase( address );
        break;
      }

      while ( !co_await finish() )
      {
        //repeat until cs is down
      }
    }
    catch ( [[maybe_unused]] NoCS const& )
    {
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
      data |= mData[address + 1];
    }
    return data;
  }
  else
  {
    int data = 0xff;
    if ( address < mData.size() )
    {
      data = mData[address];
    }
    return data;
  }
}

void EEPROM::ewen()
{
  mWriteEnable = true;
}

void EEPROM::erase( int address )
{
  write( address, 0xffff );
}

void EEPROM::write( int address, int data )
{
  if ( mWriteEnable )
  {
    if ( mDataBits == 16 )
    {
      address <<= 1;
      if ( address < mData.size() )
      {
        mData[address] = data & 0xff;
        mData[address + 1] = ( data >> 8 ) & 0xff;
      }
    }
    else
    {
      if ( address < mData.size() )
      {
        mData[address] = data & 0xff;
      }
    }
  }

  mIO.busyUntil = mIO.currentTick + WRITE_TICKS;
}

void EEPROM::eral()
{
  if ( mWriteEnable )
  {
    std::fill( mData.begin(), mData.end(), 0xff );
  }

  mIO.busyUntil = mIO.currentTick + ERAL_TICKS;
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
    }
    else
    {
      std::fill( mData.begin(), mData.end(), data & 0xff );
    }
  }

  mIO.busyUntil = mIO.currentTick + WRAL_TICKS;
}

void EEPROM::ewds()
{
  mWriteEnable = false;
}

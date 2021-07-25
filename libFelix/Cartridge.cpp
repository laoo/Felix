#include "pch.hpp"
#include "Cartridge.hpp"
#include "GameDrive.hpp"

Cartridge::Cartridge( std::shared_ptr<ImageCart const> cart ) : mCart{ std::move( cart ) }, mGameDrive{ mCart->sd() ? std::make_unique<GameDrive>( mCart->path() ) : std::unique_ptr<GameDrive>() },
  mShiftRegister{}, mCounter{}, mAudIn{}, mCurrentStrobe{}, mAddressData{},
  mBank0{ mCart->getBank0() }, mBank1{ mCart->getBank1() }
{
}

Cartridge::~Cartridge()
{
}

bool Cartridge::getAudIn( uint64_t tick ) const
{
  return mGameDrive ? mGameDrive->hasOutput( tick ) : mAudIn;
}

void Cartridge::setAudIn( bool value )
{
  //TODO: here add support for GD classic storage selection
  if ( mGameDrive )
    return;

  if ( mAudIn == value )
    return;

  mAudIn = value;

  if ( mAudIn )
  {
    auto bank0a = mCart->getBank0A();
    auto bank1a = mCart->getBank1A();

    if ( !bank0a.empty() )
      mBank0 = bank0a;

    if ( !bank1a.empty() )
      mBank1 = bank1a;
  }
  else
  {
    mBank0 = mCart->getBank0();
    mBank1 = mCart->getBank1();
  }
}

void Cartridge::setCartAddressData( bool value )
{
  mAddressData = value;
}

void Cartridge::setCartAddressStrobe( bool value )
{
  if ( value ) mCounter = 0;

  if ( value && !mCurrentStrobe )
  {
    mShiftRegister <<= 1;
    mShiftRegister += mAddressData ? 1 : 0;
    mShiftRegister &= 0xff;
  }

  mCurrentStrobe = value;
}

void Cartridge::setPower( bool value )
{
}

uint8_t Cartridge::peekRCART0( uint64_t tick )
{
  if ( mGameDrive && mGameDrive->running() )
  {
    return mGameDrive->get( tick );
  }
  else
  {
    return peek( mBank0 );
  }
}

uint8_t Cartridge::peekRCART1( uint64_t tick )
{
  return peek( mBank1 );
}

void Cartridge::pokeRCART0( uint8_t value )
{
}

void Cartridge::pokeRCART1( uint8_t value )
{
  if ( mGameDrive )
  {
    return mGameDrive->put( value );
  }
}

uint8_t Cartridge::peek( CartBank const & bank )
{
  uint8_t result = bank( mShiftRegister, mCounter );
  if ( !mCurrentStrobe )
  {
    //it's 11 bits, but it's masked anyway
    mCounter++;
  }
  return result;
}

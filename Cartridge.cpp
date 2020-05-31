#include "Cartridge.hpp"

Cartridge::Cartridge( std::shared_ptr<ImageCart const> cart ) : mCart{ std::move( cart ) },
  mShiftRegister{}, mCounter{}, mAudIn{}, mCurrentStrobe{}, mAddressData{},
  mBank0{ mCart->getBank0() }, mBank1{ mCart->getBank1() }
{
}

bool Cartridge::getAudIn() const
{
  return mAudIn;
}

void Cartridge::setAudIn( bool value )
{
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

uint8_t Cartridge::peekRCART0()
{
  return peek( mBank0 );
}

uint8_t Cartridge::peekRCART1()
{
  return peek( mBank1 );
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

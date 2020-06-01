#include "ParallelPort.hpp"
#include "Cartridge.hpp"
#include "ComLynx.hpp"
#include "DisplayGenerator.hpp"
#include "BusMaster.hpp"


ParallelPort::ParallelPort( BusMaster & busMaster, DisplayGenerator const& displayGenerator ) : mBusMaster{ busMaster }, mDisplayGenerator{ displayGenerator },
  mOutputMask{}, mData{}
{
}

void ParallelPort::setDirection( uint8_t value )
{
  mOutputMask = value;
}

uint8_t ParallelPort::getDirection() const
{
  return mOutputMask & 0x1f;
}

void ParallelPort::setData( uint8_t value )
{
  mData = value;

  if ( ( mOutputMask & Mask::CART_ADDR_DATA ) != 0 )
  {
    mBusMaster.getCartridge().setCartAddressData( ( mData & Mask::CART_ADDR_DATA ) != 0 );
    mBusMaster.getCartridge().setPower( ( mData & Mask::CART_ADDR_DATA ) == 0 );
  }

  if ( ( mOutputMask & Mask::AUDIN ) != 0 )
  {
    mBusMaster.getCartridge().setAudIn( ( mData & Mask::AUDIN ) != 0 );
  }
}

uint8_t ParallelPort::getData() const
{
  uint8_t result{};

  if ( ( mOutputMask & Mask::AUDIN ) == 0 )
  {
    result |= mBusMaster.getCartridge().getAudIn() ? Mask::AUDIN : 0;
  }
  else
  {
    result |= mData & Mask::AUDIN;
  }

  if ( ( mOutputMask & Mask::RESTLESS ) == 0 )
  {
    result |= ( ( mData & Mask::RESTLESS ) == 0 ) && !mDisplayGenerator.rest() ? Mask::RESTLESS : 0;
  }
  else
  {
    result |= mData & Mask::RESTLESS;
  }

  if ( ( mOutputMask & Mask::NOEXP ) == 0 )
  {
    result |=mBusMaster.getComLynx().present() ? Mask::NOEXP : 0;
  }
  else
  {
    result |= mData & Mask::NOEXP;
  }

  if ( ( mOutputMask & Mask::CART_ADDR_DATA ) != 0 )
  {
    result |= mData & Mask::CART_ADDR_DATA;
  }

  if ( ( mOutputMask & Mask::EXTERNAL_POWER ) == 0 )
  {
    result |= Mask::EXTERNAL_POWER;
  }
  else
  {
    result |= mData & Mask::EXTERNAL_POWER;
  }

  return result;
}

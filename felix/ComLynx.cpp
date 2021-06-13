#include "pch.hpp"
#include "ComLynx.hpp"
#include "Utility.hpp"
#include "ComLynxWire.hpp"

ComLynx::ComLynx( std::shared_ptr<ComLynxWire> comLynxWire ) : mWire{std::move( comLynxWire ) }, mWriteCtrl {}, mReadCtrl{ SERCTL::TXRDY | SERCTL::TXEMPTY }, mShift{}, mHold{}, mRead{}, mCycle{}
{
}

bool ComLynx::pulse()
{
  if ( mCycle == 0 )
  {
    switch ( mReadCtrl & ( SERCTL::TXRDY | SERCTL::RXRDY | SERCTL::TXEMPTY ) )
    {
    case ( SERCTL::TXRDY | SERCTL::RXRDY | SERCTL::TXEMPTY ): //hold empty, read ready, not trasmitting
      //hold empty, read ready, not trasmitting
      break;
    case ( SERCTL::TXRDY | SERCTL::RXRDY ): //hold empty, read ready, tansmitting
      mReadCtrl |= SERCTL::OVERRUN | SERCTL::TXEMPTY;
      setRead( mShift );
      //hold empty, read overrun, no tansmitting
      break;
    case ( SERCTL::TXRDY | SERCTL::TXEMPTY ): //hold empty, no read, not trasmitting
      //hold empty, no read, not trasmitting
      break;
    case ( SERCTL::TXRDY ): //hold empty, no read, tansmitting
      mReadCtrl |= SERCTL::RXRDY | SERCTL::TXEMPTY;
      setRead( mShift );
      //hold empty, read rady, no tansmitting
      break;
    case ( SERCTL::RXRDY | SERCTL::TXEMPTY ):  //hold full, read ready, not transmitting
      mReadCtrl |= SERCTL::TXRDY;
      mReadCtrl &= ~SERCTL::TXEMPTY;
      mShift = mHold;
      //hold empty, read ready, trasmitting
      break;
    case ( SERCTL::RXRDY ): //hold full, read ready, transmitting
      mReadCtrl |= SERCTL::TXRDY | SERCTL::OVERRUN;
      setRead( mShift );
      mShift = mHold;
      //hold empty, read overrun, transmitting
      break;
    case ( SERCTL::TXEMPTY ):  //hold full, no read, not transmitting
      mReadCtrl |= SERCTL::TXRDY;
      mReadCtrl &= ~SERCTL::TXEMPTY;
      mShift = mHold;
      //hold empty, no read, transmitting
      break;
    default:  //hold full, no read, transmitting
      mReadCtrl |= SERCTL::TXRDY | SERCTL::RXRDY;
      setRead( mShift );
      mShift = mHold;
      //hold empty, read ready, transmitting
      break;
    }
  }
  else if ( ( mReadCtrl & ( SERCTL::TXRDY | SERCTL::TXEMPTY ) ) == SERCTL::TXEMPTY )
  {
    mReadCtrl |= SERCTL::TXRDY;
    mReadCtrl &= ~SERCTL::TXEMPTY;
    mShift = mHold;
    mCycle = 0; //resetting cycle
  }

  mCycle = ( mCycle + 1 ) % 11;

  const int rx = mWriteCtrl & mReadCtrl & SERCTL::RXRDY;
  const int tx = mWriteCtrl & mReadCtrl & SERCTL::TXRDY;

  if ( rx | tx )
  {
    return true;
  }
  else
  {
    return false;
  }
}

void ComLynx::setCtrl( uint8_t value )
{
  mWriteCtrl = value;
  if ( ( value & SERCTL::RESETERR ) != 0 )
    mReadCtrl &= ~( SERCTL::PARERR | SERCTL::OVERRUN | SERCTL::FRAMERR );
}

void ComLynx::setData( uint8_t data )
{
  if ( ( mReadCtrl & SERCTL::TXRDY ) != 0 )
  {
    mHold = data;
    mReadCtrl &= ~( SERCTL::TXRDY );
  }
  else
  {
    //TODO: check
  }
}

uint8_t ComLynx::getCtrl() const
{
  return mReadCtrl;
}

uint8_t ComLynx::getData()
{
  mReadCtrl &= ~SERCTL::RXRDY;
  return mRead;
}

bool ComLynx::present() const
{
  return mWire->connected;
}

void ComLynx::setRead( uint8_t shift )
{
  if ( ( mWriteCtrl & SERCTL::PAREN ) != 0 )
  {
    mReadCtrl |= ( popcnt( shift ) + ( mWriteCtrl & SERCTL::PAREVEN ) ) & SERCTL::PAREVEN;
  }
  else
  {
    mReadCtrl |= mWriteCtrl & SERCTL::PAREVEN;
  }
  mRead = shift;
}

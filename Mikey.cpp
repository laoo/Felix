#include "Mikey.hpp"
#include <cassert>
#include "TimerCore.hpp"
#include "BusMaster.hpp"

Mikey::Mikey( BusMaster & busMaster ) : mBusMaster{ busMaster }, mAccessTick {}, mTimers{}, mDisplayGenerator{ std::make_unique<DisplayGenerator>() }, mDisplayRegs{}, mSerCtl{}, mSerDat{}, mIRQ{}
{
  mTimers[0x0] = std::make_unique<TimerCore>( 0x0, [this]( uint64_t tick, bool interrupt )
  {
    mTimers[0x2]->borrowIn( tick );
    uint8_t cnt = mTimers[0x02]->getCount( tick );
    if ( cnt == 101 )
    {
      mDisplayGenerator->updateDispAddr( mDisplayRegs.dispAdr );
    }
    if ( auto dma = mDisplayGenerator->hblank( tick, cnt ) )
    {
      mBusMaster.requestDisplayDMA( dma.tick, dma.address );
    }
    mIRQ |= interrupt ? 0x01 : 0x00;
  } );  //timer 0 -> timer 2
  mTimers[0x1] = std::make_unique<TimerCore>( 0x1, [this]( uint64_t tick, bool interrupt )
  {
    mTimers[0x3]->borrowIn( tick );
    mIRQ |= interrupt ? 0x02 : 0x00;
  } );  //timer 1 -> timer 3
  mTimers[0x2] = std::make_unique<TimerCore>( 0x2, [this]( uint64_t tick, bool interrupt )
  {
    mTimers[0x4]->borrowIn( tick );
    mDisplayGenerator->vblank( tick );
    mIRQ |= interrupt ? 0x04 : 0x00;
  } );  //timer 2 -> timer 4
  mTimers[0x3] = std::make_unique<TimerCore>( 0x3, [this]( uint64_t tick, bool interrupt )
  {
    mTimers[0x5]->borrowIn( tick );
    mIRQ |= interrupt ? 0x08 : 0x00;
  } );  //timer 3 -> timer 5
  mTimers[0x4] = std::make_unique<TimerCore>( 0x4, [this]( uint64_t tick, bool interrupt )
  {
  } );  //timer 4
  mTimers[0x5] = std::make_unique<TimerCore>( 0x5, [this]( uint64_t tick, bool interrupt )
  {
    mTimers[0x7]->borrowIn( tick );
    mIRQ |= interrupt ? 0x20 : 0x00;
  } );  //timer 5 -> timer 7
  mTimers[0x6] = std::make_unique<TimerCore>( 0x6, [this]( uint64_t tick, bool interrupt )
  {
    mIRQ |= interrupt ? 0x40 : 0x00;
  } );  //timer 6
  mTimers[0x7] = std::make_unique<TimerCore>( 0x7, [this]( uint64_t tick, bool interrupt )
  {
    mTimers[0x8]->borrowIn( tick );
    mIRQ |= interrupt ? 0x80 : 0x00;
  } );  //timer 7 -> audio 0
  mTimers[0x8] = std::make_unique<TimerCore>( 0x8, [this]( uint64_t tick, bool unused )
  {
    mTimers[0x9]->borrowIn( tick );
  } );  //audio 0 -> audio 1
  mTimers[0x9] = std::make_unique<TimerCore>( 0x9, [this]( uint64_t tick, bool unused )
  {
    mTimers[0xa]->borrowIn( tick );
  } );  //audio 1 -> audio 2
  mTimers[0xa] = std::make_unique<TimerCore>( 0xa, [this]( uint64_t tick, bool unused )
  {
    mTimers[0xb]->borrowIn( tick );
  } );  //audio 2 -> audio 3
  mTimers[0xb] = std::make_unique<TimerCore>( 0xb, [this]( uint64_t tick, bool unused )
  {
    mTimers[0x0]->borrowIn( tick );
  } );  //audio 3 -> timer 1
}

Mikey::~Mikey()
{
}

uint64_t Mikey::requestAccess( uint64_t tick, uint16_t address )
{
  mAccessTick = tick + 5;

  address &= 0xff;
  int timer = -1;

  if ( address < 0x20 )
  {
    timer = ( address & 0x1f ) >> 2;
  }
  else if ( address < 0x40 )
  {
    timer = 8 + ( ( ( address - 0x20 ) & 0x1f ) >> 3 );
  }

  if ( timer >= 0 )
  {
    auto nextTick = mAccessTick & ~0xfull + ( timer * 16 / 12 );
    mAccessTick = nextTick < mAccessTick ? nextTick + 16 : nextTick;
  }
  
  return mAccessTick;
}

uint8_t Mikey::read( uint16_t address )
{
  address &= 0xff;

  if ( address < 0x20 )
  {
    switch ( address & 0x3 )
    {
    case TIMER::BACKUP:
      return mTimers[( address >> 2 ) & 7]->getBackup( mAccessTick );
      break;
    case TIMER::CONTROLA:
      return mTimers[( address >> 2 ) & 7]->getControlA( mAccessTick );
      break;
    case TIMER::COUNT:
      return mTimers[( address >> 2 ) & 7]->getCount( mAccessTick );
      break;
    case TIMER::CONTROLB:
      return mTimers[( address >> 2 ) & 7]->getControlB( mAccessTick );
      break;
    }
  }
  else if ( address < 0x40 )
  {
    switch ( address & 0x7 )
    {
    case AUDIO::VOLCNTRL:
      break;
    case AUDIO::FEEDBACK:
      break;
    case AUDIO::OUTPUT:
      break;
    case AUDIO::SHIFT:
      break;
    case AUDIO::BACKUP:
      break;
    case AUDIO::CONTROL:
      break;
    case AUDIO::COUNTER:
      break;
    case AUDIO::OTHER:
      break;
    }
  }
  else switch ( address )
  {
  case INTRST:
  case INTSET:
    return mIRQ;
  case MIKEYHREV:
    return 0x01;
  case SERCTL:
    return
      ( mSerCtl.txrdy ? 0x80 : 0x00 ) |
      ( mSerCtl.rxrdy ? 0x40 : 0x00 ) |
      ( mSerCtl.txempty ? 0x20 : 0x00 ) |
      ( mSerCtl.parerr ? 0x10 : 0x00 ) |
      ( mSerCtl.overrun ? 0x08 : 0x00 ) |
      ( mSerCtl.framerr ? 0x04 : 0x00 ) |
      ( mSerCtl.rxbrk ? 0x02 : 0x00 ) |
      ( mSerCtl.parbit ? 0x01 : 0x00 );
  default:
    assert( false );
    break;
  }

  return uint8_t();
}

Mikey::WriteAction Mikey::write( uint16_t address, uint8_t value )
{
  SequencedAction result;
  address &= 0xff;

  if ( address < 0x20 )
  {
    switch ( address & 0x3 )
    {
    case TIMER::BACKUP:
      result = mTimers[(address >> 2) & 7]->setBackup( mAccessTick, value );
      break;
    case TIMER::CONTROLA:
      result = mTimers[( address >> 2 ) & 7]->setControlA( mAccessTick, value );
      break;
    case TIMER::COUNT:
      result = mTimers[( address >> 2 ) & 7]->setCount( mAccessTick, value );
      break;
    case TIMER::CONTROLB:
      result = mTimers[( address >> 2 ) & 7]->setControlB( mAccessTick, value );
      break;
    }
  }
  else if ( address < 0x40 )
  {
    switch ( address & 0x7 )
    {
    case AUDIO::VOLCNTRL:
      break;
    case AUDIO::FEEDBACK:
      break;
    case AUDIO::OUTPUT:
      break;
    case AUDIO::SHIFT:
      break;
    case AUDIO::BACKUP:
      break;
    case AUDIO::CONTROL:
      break;
    case AUDIO::COUNTER:
      break;
    case AUDIO::OTHER:
      break;
    }
  }
  else switch ( address )
  {
  case MPAN:
  case MSTEREO:
    break;
  case INTRST:
    mIRQ &= ~value;
    break;
  case INTSET:
    mIRQ |= ~value;
    break;
  case IODIR:
  case IODAT:
    break;
  case SERCTL:
    mSerCtl.txinten = ( value & 0x80 ) != 0;
    mSerCtl.rxinten = ( value & 0x40 ) != 0;
    mSerCtl.paren = ( value & 0x10 ) != 0;
    mSerCtl.txopen = ( value & 0x04 ) != 0;
    mSerCtl.txbrk = ( value & 0x02 ) != 0;
    mSerCtl.pareven = ( value & 0x01 ) != 0;
    if ( ( value & 0x08 ) != 0 )
      mSerCtl.parerr = mSerCtl.overrun = mSerCtl.framerr = false;
    break;
  case SDONEACK:
    mSuzyDone = false;
    break;
  case CPUSLEEP:
    if ( !mSuzyDone )
    {
      return { WriteAction::Type::START_SUZY };
    }
    break;
  case DISPCTL:
    mDisplayRegs.dispColor = ( value & DISPCTL::DISP_COLOR ) != 0;
    mDisplayRegs.dispFourBit = ( value & DISPCTL::DISP_FOURBIT ) != 0;
    mDisplayRegs.dispFlip = ( value & DISPCTL::DISP_FLIP ) != 0;
    mDisplayRegs.DMAEnable = ( value & DISPCTL::DMA_ENABLE ) != 0;
    mDisplayGenerator->dispCtl( mDisplayRegs.dispColor, mDisplayRegs.dispFlip, mDisplayRegs.DMAEnable );
    break;
  case PBKUP:
    mDisplayRegs.pbkup = value;
    break;
  case DISPADR:
    mDisplayRegs.dispAdr &= 0xff00;
    mDisplayRegs.dispAdr |= value;
    mDisplayRegs.dispAdr &= 0xfffc;
    break;
  case DISPADR+1:
    mDisplayRegs.dispAdr &= 0x00ff;
    mDisplayRegs.dispAdr |= value << 8;
    break;
  case MTEST0:
    break;
  case MTEST1:
    break;
  case MTEST2:
    if ( ( value & 0x01 ) != 0 )
      mDisplayGenerator->updateDispAddr( mDisplayRegs.dispAdr );
    break;
  case GREEN + 0x00:
  case GREEN + 0x01:
  case GREEN + 0x02:
  case GREEN + 0x03:
  case GREEN + 0x04:
  case GREEN + 0x05:
  case GREEN + 0x06:
  case GREEN + 0x07:
  case GREEN + 0x08:
  case GREEN + 0x09:
  case GREEN + 0x0a:
  case GREEN + 0x0b:
  case GREEN + 0x0c:
  case GREEN + 0x0d:
  case GREEN + 0x0e:
  case GREEN + 0x0f:
  case BLUERED + 0x00:
  case BLUERED + 0x01:
  case BLUERED + 0x02:
  case BLUERED + 0x03:
  case BLUERED + 0x04:
  case BLUERED + 0x05:
  case BLUERED + 0x06:
  case BLUERED + 0x07:
  case BLUERED + 0x08:
  case BLUERED + 0x09:
  case BLUERED + 0x0a:
  case BLUERED + 0x0b:
  case BLUERED + 0x0c:
  case BLUERED + 0x0d:
  case BLUERED + 0x0e:
  case BLUERED + 0x0f:
    mPalette[address - GREEN] = value;
    mDisplayGenerator->updatePalette( mAccessTick, address - GREEN, value );
    break;
  default:
    assert( false );
    break;
  }

  if ( result )
  {
    return { WriteAction::Type::FIRE_TIMER, result };
  }
  else
  {
    return {};
  }
}

SequencedAction Mikey::fireTimer( uint64_t tick, uint32_t timer )
{
  assert( timer < 12 );
  return mTimers[timer]->fireAction( tick );
}

void Mikey::setDMAData( uint64_t tick, uint64_t data )
{
  if ( auto dma = mDisplayGenerator->pushData( tick, data ) )
  {
    mBusMaster.requestDisplayDMA( dma.tick, dma.address );
  }
}

uint8_t Mikey::getIRQ() const
{
  return mIRQ;
}

void Mikey::suzyDone()
{
  mSuzyDone = true;
}

DisplayGenerator::Pixel const * Mikey::getSrface() const
{
  return mDisplayGenerator->getSrface();
}

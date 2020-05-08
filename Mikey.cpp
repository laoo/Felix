#include "Mikey.hpp"
#include <cassert>
#include "TimerCore.hpp"

Mikey::Mikey() : mAccessTick{}, mTimers{}, mDisplayRegs{}
{
  mTimers[0xb] = std::make_unique<TimerCore>( 0xe, nullptr );  //audio 3 //should link to timer 1
  mTimers[0xa] = std::make_unique<TimerCore>( 0xd, &*mTimers[0xb] );  //audio 2 -> audio 3
  mTimers[0x9] = std::make_unique<TimerCore>( 0xc, &*mTimers[0xa] );  //audio 1 -> audio 2
  mTimers[0x8] = std::make_unique<TimerCore>( 0xa, &*mTimers[0x9] );  //audio 0 -> audio 1
  mTimers[0x7] = std::make_unique<TimerCore>( 0x9, &*mTimers[0x8] );  //timer 7 -> audio 0
  mTimers[0x6] = std::make_unique<TimerCore>( 0x8, nullptr );  //timer 6
  mTimers[0x5] = std::make_unique<TimerCore>( 0x6, &*mTimers[0x7] );  //timer 5 -> timer 7
  mTimers[0x4] = std::make_unique<TimerCore>( 0x5, nullptr );  //timer 4
  mTimers[0x3] = std::make_unique<TimerCore>( 0x4, &*mTimers[0x5] );  //timer 3 -> timer 5
  mTimers[0x2] = std::make_unique<TimerCore>( 0x2, &*mTimers[0x4] );  //timer 2 -> timer 4
  mTimers[0x1] = std::make_unique<TimerCore>( 0x1, &*mTimers[0x3] );  //timer 1 -> timer 3
  mTimers[0x0] = std::make_unique<TimerCore>( 0x0, &*mTimers[0x2] );  //timer 0 -> timer 2
}

Mikey::~Mikey()
{
}

uint64_t Mikey::requestAccess( uint64_t tick, uint16_t address )
{
  mAccessTick = tick + 5;
  return mAccessTick;
}

uint8_t Mikey::read( uint16_t address )
{
  switch ( address )
  {
  default:
    assert( false );
  }

  return uint8_t();
}

SequencedAction Mikey::write( uint16_t address, uint8_t value )
{
  address &= 0xff;

  if ( address < 0x20 )
  {
    switch ( address & 0x3 )
    {
    case Reg::Offset::TIMER::BACKUP:
      return mTimers[(address >> 2) & 7]->setBackup( value );
      break;
    case Reg::Offset::TIMER::CONTROLA:
      return mTimers[( address >> 2 ) & 7]->setControlA( value );
      break;
    case Reg::Offset::TIMER::COUNT:
      return mTimers[( address >> 2 ) & 7]->setCount( value );
      break;
    case Reg::Offset::TIMER::CONTROLB:
      return mTimers[( address >> 2 ) & 7]->setControlB( value );
      break;
    }
  }
  else if ( address < 0x40 )
  {
    switch ( address & 0x7 )
    {
    case Reg::Offset::AUDIO::VOLCNTRL:
      break;
    case Reg::Offset::AUDIO::FEEDBACK:
      break;
    case Reg::Offset::AUDIO::OUTPUT:
      break;
    case Reg::Offset::AUDIO::SHIFT:
      break;
    case Reg::Offset::AUDIO::BACKUP:
      break;
    case Reg::Offset::AUDIO::CONTROL:
      break;
    case Reg::Offset::AUDIO::COUNTER:
      break;
    case Reg::Offset::AUDIO::OTHER:
      break;
    }
  }
  else switch ( address )
  {
  case Reg::Offset::DISPCTL:
    mDisplayRegs.dispColor = ( value & Reg::DISPCTL::DISP_COLOR ) != 0;
    mDisplayRegs.dispFourBit = ( value & Reg::DISPCTL::DISP_FOURBIT ) != 0;
    mDisplayRegs.dispFlip = ( value & Reg::DISPCTL::DISP_FLIP ) != 0;
    mDisplayRegs.DMAEnable = ( value & Reg::DISPCTL::DMA_ENABLE ) != 0;
    break;
  default:
    assert( false );
    break;
  }

  return {};
}

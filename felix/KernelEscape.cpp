#include "pch.hpp"
#include "KernelEscape.hpp"
#include "CPUState.hpp"
#include "Encryption.hpp"
#include "Log.hpp"

KernelEscape::KernelEscape()
{
}

void KernelEscape::call( uint8_t data, IAccessor & acc )
{
  switch ( data )
  {
  case 0xff:
    reset( acc );
    [[fallthrough]];
  case 0xfd:
    clear( acc );
    [[fallthrough]];
  case 0xfe:
    decryptCartridge( acc );
    break;
  case 0xfc:
    shift( acc );
    break;
  default:
    break;
  }
}

void KernelEscape::shift( IAccessor & acc )
{
  acc.writeMikey( 0x87, 2 );  //SYSCTL1

  uint8_t value = acc.state().a;
  for ( int i = 0; i < 8; ++i )
  {
    acc.writeMikey( 0x8b, ( value & 0x80 ) != 0 ? 2 : 0 );  //IODAT
    value <<= 1;
    acc.writeMikey( 0x87, 3 );  //SYSCTL1
    acc.writeMikey( 0x87, 2 );  //SYSCTL1
  }
}

void KernelEscape::clear( IAccessor & acc )
{
  for ( int i = 0; i < 0x10000; ++i )
  {
    acc.writeRAM( (uint16_t)i, 0 );
  }

  acc.writeRAM( 5, 0x00 );
  acc.writeRAM( 6, 0x02 );

  acc.writeMikey( 0x00, 0x9e ); //TIM0BCKUP
  acc.writeMikey( 0x01, 0x18 ); //TIM0CTLA
  acc.writeMikey( 0x08, 0x68 ); //TIM2BCKUP
  acc.writeMikey( 0x09, 0x1f ); //TIM2CTLA
  acc.writeMikey( 0x93, 0x29 ); //PBCKUP
  acc.writeMikey( 0x92, 0x0d ); //DISPCTL
  acc.writeMikey( 0x90, 0x00 ); //SDONEACK

  acc.state().a = 0; 
  shift( acc );
}

void KernelEscape::decryptCartridge( IAccessor & acc )
{
  uint16_t addr = acc.readRAM( 5 ) + ( (uint16_t)acc.readRAM( 6 ) << 8 );

  size_t blockcount = 0x100 - acc.readSuzy( 0xb2 );

  if ( blockcount > 5 )
  {
    L_ERROR << "Bad number of encrypted blocks: " << blockcount;
    return;
  }

  std::vector<uint8_t> enc;
  for ( size_t i = 0; i < 51 * blockcount; ++i )
  {
    enc.push_back( acc.readSuzy( 0xb2 ) );
  }

  auto plain = decrypt( blockcount, std::span<uint8_t const>{ enc.data(), enc.size() } );

  assert( plain.size() <= 50 * blockcount );

  for ( int i = 0; i < 50 * blockcount; ++i )
  {
    acc.writeRAM( addr++, plain[i] );
  }

  acc.state().pc = 0x200;
}

void KernelEscape::reset( IAccessor & acc )
{
  acc.writeMikey( 0x8b, 2 );  //IODAT
  acc.writeMikey( 0x8a, 3 );  //IODIR
}


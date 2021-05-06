#include "pch.hpp"
#include "KernelEscape.hpp"
#include "CPUState.hpp"
#include <boost/multiprecision/cpp_int.hpp>

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
    decrypt( acc );
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

void KernelEscape::decrypt( IAccessor & acc )
{
  uint16_t addr = acc.readRAM( 5 ) + ( (uint16_t)acc.readRAM( 6 ) << 8 );

  std::array<uint8_t, 256> enc;

  enc[0] = acc.readSuzy( 0xb2 );

  if ( enc[0] < 0xfb )
    return;

  size_t blockcount = 0x100 - enc[0];
  for ( size_t i = 1; i < 1 + 51 * blockcount; ++i )
  {
    enc[i] = acc.readSuzy( 0xb2 );
  }

  auto plain = decrypt( blockcount, std::span<uint8_t const>{ enc.data() + 1, 255 } );

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

std::vector<uint8_t> KernelEscape::decrypt( size_t blockcount, std::span<uint8_t const> encrypted )
{
  std::vector<uint8_t> result;
  int accumulator = 0;
  for ( size_t i = 0; i < blockcount; ++i )
  {
    decrypt( std::span<uint8_t const>{ encrypted.data() + 51 * i, 51 }, accumulator, result );
  }

  return result;
}

void KernelEscape::decrypt( std::span<uint8_t const> encrypted, int & accumulator, std::vector<uint8_t> & result )
{
  using namespace boost::multiprecision;
  using namespace boost::multiprecision::literals;

  uint512_t constexpr lynxpubmod = 0x35b5a3942806d8a22695d771b23cfd561c4a19b6a3b02600365a306e3c4d63381bd41c136489364cf2ba2a58f4fee1fdac7e79_cppui512;
  uint512_t constexpr lynxprvexp = 0x23ce6d0d7004906c19b93a4bcc28a8e412dc11246d2019557987ab5ca818a3d3c8e3276d4270cb8021d6bda4296d47b1e5e2a3_cppui512;
  uint512_t constexpr lynxpubexp = 3;
  uint512_t enc;
  import_bits( enc, encrypted.begin(), encrypted.end(), 8, false );
  uint512_t decr = powm( enc, lynxpubexp, lynxpubmod );
  std::vector<uint8_t> decrv;
  export_bits( decr, std::back_inserter( decrv ), 8, false );

  for ( size_t i = 0; i < decrv.size() - 1; ++i ) //skipping last byte
  {
    accumulator += decrv[i];
    result.push_back( accumulator );
  }
}


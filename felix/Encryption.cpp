#include "pch.hpp"
#include "Encryption.hpp"
#include "Log.hpp"

uint8_t decrypt( std::span<uint8_t const> encrypted, int& accumulator, std::vector<uint8_t>& result )
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

  return decrv.back();
}

std::vector<uint8_t> decrypt( size_t blockcount, std::span<uint8_t const> encrypted )
{
  std::vector<uint8_t> result;
  int accumulator = 0;
  for ( size_t i = 0; i < blockcount; ++i )
  {
    uint8_t sanityChek = decrypt( std::span<uint8_t const>{ encrypted.data() + 51 * i, 51 }, accumulator, result );
    if ( sanityChek != 0x15 )
    {
      L_ERROR << "Sanity check #1 value for block " << i << " is 0x" << std::hex << sanityChek << " != 0x15";
      return {};
    }
  }

  if ( ( accumulator & 0xff ) != 0 )
  {
    L_ERROR << "Sanity check #2 final accumulator value 0x" << std::hex << ( accumulator & 0xff ) << " != 0x00";
    return {};

  }
  return result;
}

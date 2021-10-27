#include "pch.hpp"
#include "EEPROM.hpp"
#include "ImageCart.hpp"

EEPROM::EEPROM( std::filesystem::path imagePath, int eeType, bool is16Bit ) : mEECoroutine{ process() }
{
  assert( eeType != 0 );
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

void EEPROM::tick()
{
}

EEPROM::EECoroutine EEPROM::process()
{
  co_return;
}


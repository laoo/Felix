#pragma once

#include "ImageCart.hpp"

class Cartridge
{
public:
  Cartridge( std::shared_ptr<ImageCart const> cart );

  bool getAudIn() const;
  void setAudIn( bool value );

  void setCartAddressData( bool value );
  void setCartAddressStrobe( bool value );

  void setPower( bool value );

  uint8_t peekRCART0();
  uint8_t peekRCART1();

private:
  uint8_t peek( CartBank const& bank );


  std::shared_ptr<ImageCart const> mCart;
  uint32_t mShiftRegister;
  uint16_t mCounter;
  bool mAudIn;
  bool mCurrentStrobe;
  bool mAddressData;
  CartBank mBank0;
  CartBank mBank1;
};

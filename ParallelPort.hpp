#pragma once
#include <cstdint>

class Cartridge;
class ComLynx;
class DisplayGenerator;

class ParallelPort
{
public:

  ParallelPort( Cartridge & cart, ComLynx & comLynx, DisplayGenerator const& displayGenerator );

  void setDirection( uint8_t value );
  uint8_t getDirection() const;

  void setData( uint8_t value );
  uint8_t getData() const;

  struct Mask
  {
    static constexpr uint8_t AUDIN          = 0b00010000; 
    static constexpr uint8_t RESTLESS       = 0b00001000;
    static constexpr uint8_t NOEXP          = 0b00000100;
    static constexpr uint8_t CART_ADDR_DATA = 0b00000010;
    static constexpr uint8_t CART_POWER_OFF = 0b00000010;
    static constexpr uint8_t EXTERNAL_POWER = 0b00000001;
  };



private:
  Cartridge & mCart;
  ComLynx & mComLynx;
  DisplayGenerator const& mDisplayGenerator;
  uint8_t mOutputMask;
  uint8_t mData;
};

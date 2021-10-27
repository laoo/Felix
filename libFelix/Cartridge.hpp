#pragma once

#include "ImageCart.hpp"

class GameDrive;
class EEPROM;
class TraceHelper;

class Cartridge
{
public:
  Cartridge( std::shared_ptr<ImageCart const> cart, std::shared_ptr<TraceHelper> traceHelper );
  ~Cartridge();

  bool getAudIn( uint64_t tick ) const;
  void setAudIn( bool value );

  void setCartAddressData( bool value );
  void setCartAddressStrobe( bool value );

  void setPower( bool value );

  uint8_t peekRCART0( uint64_t tick );
  uint8_t peekRCART1( uint64_t tick );

  void pokeRCART0( uint64_t tick, uint8_t value );
  void pokeRCART1( uint64_t tick, uint8_t value );

private:
  uint8_t peek( CartBank const& bank );

  void incrementCounter( uint64_t tick );

  std::array<char, 256> mCommentBuffer;
  std::shared_ptr<ImageCart const> mCart;
  std::unique_ptr<GameDrive> mGameDrive;
  std::unique_ptr<EEPROM> mEEPROM;
  std::shared_ptr<TraceHelper> mTraceHelper;
  uint32_t mShiftRegister;
  uint16_t mCounter;
  bool mAudIn;
  bool mCurrentStrobe;
  bool mAddressData;
  CartBank mBank0;
  CartBank mBank1;
};

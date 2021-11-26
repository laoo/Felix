#pragma once

class KeyInput
{
  uint32_t data;
public:

  enum Key : uint32_t
  {
    OUTER   = 0,
    INNER   = 1,
    OPTION2 = 2,
    OPTION1 = 3,
    RIGHT   = 4,
    LEFT    = 5,
    DOWN    = 6,
    UP      = 7,
    PAUSE   = 8
  };

  constexpr uint32_t bitmask( Key k ) const
  {
    return 1 << (uint32_t)k;
  }

  bool get( Key k ) const
  {
    return ( data & bitmask( k ) ) != 0;
  }

  void set( Key k, bool value )
  {
    data |= ( data & ~bitmask( k ) ) | ( value ? bitmask( k ) : 0 );
  }

  uint8_t joystick() const
  {
    return data & 0xff;
  }

  uint8_t switches() const
  {
    return ( data >> 8 ) & 0xff;
  }
};

class IInputSource
{
public:
  virtual ~IInputSource() = default;

  virtual KeyInput getInput( bool leftHand ) const = 0;
};

#pragma once

struct KeyInput
{
  uint32_t bitmask;

  enum Key : uint32_t
  {
    PAUSE   = 0b100000000,
    DOWN    = 0b010000000,
    UP      = 0b001000000,
    RIGHT   = 0b000100000,
    LEFT    = 0b000010000,
    OPTION1 = 0b000001000,
    OPTION2 = 0b000000100,
    INNER   = 0b000000010,
    OUTER   = 0b000000001
  };

  bool get( Key k ) const
  {
    return ( bitmask & (uint32_t)k ) != 0;
  }

  void set( Key k, bool value )
  {
    bitmask |= ( bitmask & (uint32_t)k ) | ( value ? (uint32_t)k : 0 );
  }
};

class IInputSource
{
public:
  virtual ~IInputSource() = default;

  virtual KeyInput getInput( bool leftHand ) const = 0;
};

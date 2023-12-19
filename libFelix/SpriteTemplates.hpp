#pragma once
#include "Suzy.hpp"

template<Suzy::Sprite>
struct SuzySprite
{
};


/*
    BACKGROUND_SPRITE = %00000000
    BACK_SHADOW_SPRITE= %00000000

    Background-Shadow
*/
template<>
struct SuzySprite<Suzy::Sprite::BACKGROUND>
{
  //xor sprite performs always RMW on video buffer
  static constexpr bool eor = false;
  //background sprite if always opaque so does not read video buffer
  static constexpr bool background = true;
  //whether collision buffer is read and collision depository updated
  static constexpr bool collDep = false;

  //whether given pixel is opaque
  static constexpr bool opaque( int pixel )
  {
    return true;
  }

  //whether given pixel is colliding
  static constexpr bool colliding( int pixel )
  {
    return pixel != 0xe;
  }
};

/*
    BACKNONCOLL_SPRITE= %00000001

    Background-No Collision
*/
template<>
struct SuzySprite<Suzy::Sprite::BACKNONCOLL>
{
  static constexpr bool eor = false;
  static constexpr bool background = true;
  static constexpr bool collDep = false;

  static constexpr bool opaque( int pixel )
  {
    return true;
  }

  static constexpr bool colliding( int pixel )
  {
    return false;
  }
};

/*
    ;BOUNDARY_SPRITE  = %00000010
    BSHADOW_SPRITE    = %00000010

    Boundary Shadow
*/
template<>
struct SuzySprite<Suzy::Sprite::BSHADOW>
{
  static constexpr bool eor = false;
  static constexpr bool background = false;
  static constexpr bool collDep = true;

  static constexpr bool opaque( int pixel )
  {
    return pixel != 0x0 && pixel != 0xf;
  }

  static constexpr bool colliding( int pixel )
  {
    return pixel != 0xe && pixel != 0;
  }
};

/*
    ;BSHADOW_SPRITE   = %00000011
    BOUNDARY_SPRITE   = %00000011

    Boundary
*/
template<>
struct SuzySprite<Suzy::Sprite::BOUNDARY>
{
  static constexpr bool eor = false;
  static constexpr bool background = false;
  static constexpr bool collDep = true;

  static constexpr bool opaque( int pixel )
  {
    return pixel != 0x0 && pixel != 0xf;
  }

  static constexpr bool colliding( int pixel )
  {
    return pixel != 0;
  }
};


/*
    ;SHADOW_SPRITE    = %00000100
    NORMAL_SPRITE     = %00000100

    Normal
*/
template<>
struct SuzySprite<Suzy::Sprite::NORMAL>
{
  static constexpr bool eor = false;
  static constexpr bool background = false;
  static constexpr bool collDep = true;

  static constexpr bool opaque( int pixel )
  {
    return pixel != 0x0;
  }

  static constexpr bool colliding( int pixel )
  {
    return pixel != 0;
  }
};

/*
    NONCOLL_SPRITE    = %00000101

    Non-Collideable
*/
template<>
struct SuzySprite<Suzy::Sprite::NONCOLL>
{
  static constexpr bool eor = false;
  static constexpr bool background = false;
  static constexpr bool collDep = false;

  static constexpr bool opaque( int pixel )
  {
    return pixel != 0x0;
  }

  static constexpr bool colliding( int pixel )
  {
    return false;
  }
};

/*
    XOR_SPRITE        = %00000110
    XOR_SHADOW_SPRITE = %00000110

    Exclusive-or-shadow
*/
template<>
struct SuzySprite<Suzy::Sprite::XOR>
{
  static constexpr bool eor = true;
  static constexpr bool background = false;
  static constexpr bool collDep = true;

  static constexpr bool opaque( int pixel )
  {
    return true;
  }

  static constexpr bool colliding( int pixel )
  {
    return pixel != 0xe && pixel != 0;
  }
};

/*
    ;NORMAL_SPRITE    = %00000111
    SHADOW_SPRITE     = %00000111

    Shadow
*/
template<>
struct SuzySprite<Suzy::Sprite::SHADOW>
{
  static constexpr bool eor = false;
  static constexpr bool background = false;
  static constexpr bool collDep = true;

  static constexpr bool opaque( int pixel )
  {
    return pixel != 0x0;
  }

  static constexpr bool colliding( int pixel )
  {
    return pixel != 0xe && pixel != 0;
  }
};

#pragma once
#include "Suzy.hpp"

template<Suzy::Sprite>
struct SuzySprite
{
};


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

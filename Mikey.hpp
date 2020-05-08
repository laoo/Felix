#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include "ActionQueue.hpp"

class TimerCore;

class Mikey
{
public:
  Mikey();
  ~Mikey();

  uint64_t requestAccess( uint64_t tick, uint16_t address );
  uint8_t read( uint16_t address );
  SequencedAction write( uint16_t address, uint8_t value );

  struct Reg
  {
    struct Offset
    {
      struct TIMER
      {
        static constexpr uint8_t BACKUP   = 0x00;
        static constexpr uint8_t CONTROLA = 0x01;
        static constexpr uint8_t COUNT    = 0x02;
        static constexpr uint8_t CONTROLB = 0x03;
      };

      struct AUDIO
      {
        static constexpr uint8_t VOLCNTRL   = 0x0;
        static constexpr uint8_t FEEDBACK   = 0x1;
        static constexpr uint8_t OUTPUT     = 0x2;
        static constexpr uint8_t SHIFT      = 0x3;
        static constexpr uint8_t BACKUP     = 0x4;
        static constexpr uint8_t CONTROL    = 0x5;
        static constexpr uint8_t COUNTER    = 0x6;
        static constexpr uint8_t OTHER      = 0x7;
      };

      static constexpr uint8_t DISPCTL = 0x92;
    };

    struct DISPCTL
    {
      static constexpr uint8_t DISP_COLOR   = 0b00001000; //color: 1 = color, 0 = monochrome.must be set to 1 ( set by kernel )
      static constexpr uint8_t DISP_FOURBIT = 0b00000100; //fourbit: 1 = 4 bit mode, 0 = 2 bit mode.must be set to 1 ( set by kernel )
      static constexpr uint8_t DISP_FLIP    = 0b00000010; //1 = flip, 0 normal
      static constexpr uint8_t DMA_ENABLE   = 0b00000001; //1 = enable video DMA, 0 = disable.must be set to 1 ( set by kernel )
    };
  };

private:

    uint64_t mAccessTick;

    std::array<std::unique_ptr<TimerCore>, 12> mTimers;

    struct DisplayRegs
    {
      bool dispColor;
      bool dispFourBit;
      bool dispFlip;
      bool DMAEnable;
    } mDisplayRegs;

};

#pragma once

#include "ActionQueue.hpp"
#include "DisplayGenerator.hpp"
#include "ParallelPort.hpp"

class Felix;
class TimerCore;
class AudioChannel;
class DisplayGenerator;

class Mikey
{
public:
  Mikey( Felix & felix, std::function<void( DisplayGenerator::Pixel const* )> const& fun );
  ~Mikey();

  uint64_t requestAccess( uint64_t tick, uint16_t address );
  uint8_t read( uint16_t address );
  SequencedAction write( uint16_t address, uint8_t value );
  SequencedAction fireTimer( uint64_t tick, uint32_t timer );
  void setDMAData( uint64_t tick, uint64_t data );
  void suzyDone();
  std::pair<float, float> sampleAudio() const;

  void setIRQ( uint8_t mask );
  void resetIRQ( uint8_t mask );


  DisplayGenerator::Pixel const* getSrface() const;

  struct TIMER
  {
    static constexpr uint16_t BACKUP   = 0x00;
    static constexpr uint16_t CONTROLA = 0x01;
    static constexpr uint16_t COUNT    = 0x02;
    static constexpr uint16_t CONTROLB = 0x03;
  };

  struct AUDIO
  {
    static constexpr uint16_t VOLCNTRL   = 0x0;
    static constexpr uint16_t FEEDBACK   = 0x1;
    static constexpr uint16_t OUTPUT     = 0x2;
    static constexpr uint16_t SHIFT      = 0x3;
    static constexpr uint16_t BACKUP     = 0x4;
    static constexpr uint16_t CONTROL    = 0x5;
    static constexpr uint16_t COUNTER    = 0x6;
    static constexpr uint16_t OTHER      = 0x7;
  };

  static constexpr uint16_t ATTENREG0    = 0x40;
  static constexpr uint16_t ATTENREG1    = 0x41;
  static constexpr uint16_t ATTENREG2    = 0x42;
  static constexpr uint16_t ATTENREG3    = 0x43;
  static constexpr uint16_t MPAN         = 0x44;
  static constexpr uint16_t MSTEREO      = 0x50;
  static constexpr uint16_t INTRST       = 0x80;
  static constexpr uint16_t INTSET       = 0x81;
  static constexpr uint16_t SYSCTL1      = 0x87;
  static constexpr uint16_t MIKEYHREV    = 0x89;
  static constexpr uint16_t IODIR        = 0x8a;
  static constexpr uint16_t IODAT        = 0x8b;
  static constexpr uint16_t SERCTL       = 0x8c;
  static constexpr uint16_t SERDAT       = 0x8d;
  static constexpr uint16_t SDONEACK     = 0x90;
  static constexpr uint16_t CPUSLEEP     = 0x91;
  static constexpr uint16_t DISPCTL      = 0x92;
  static constexpr uint16_t PBKUP        = 0x93;
  static constexpr uint16_t DISPADR      = 0x94;
  static constexpr uint16_t MTEST0       = 0x9c;
  static constexpr uint16_t MTEST1       = 0x9d;
  static constexpr uint16_t MTEST2       = 0x9e;
  static constexpr uint16_t GREEN        = 0xa0;
  static constexpr uint16_t BLUERED      = 0xb0;

  struct SYSCTL1
  {
    static constexpr uint8_t POWERON          = 0b00000010; //writing 0 shuts down the console
    static constexpr uint8_t CART_ADDR_STROBE = 0b00000001; //
  };

  struct DISPCTL
  {
    static constexpr uint8_t DISP_COLOR   = 0b00001000; //color: 1 = color, 0 = monochrome.must be set to 1 ( set by kernel )
    static constexpr uint8_t DISP_FOURBIT = 0b00000100; //fourbit: 1 = 4 bit mode, 0 = 2 bit mode.must be set to 1 ( set by kernel )
    static constexpr uint8_t DISP_FLIP    = 0b00000010; //1 = flip, 0 normal
    static constexpr uint8_t DMA_ENABLE   = 0b00000001; //1 = enable video DMA, 0 = disable.must be set to 1 ( set by kernel )
  };

private:
  Felix & mFelix;
  uint64_t mAccessTick;

  std::array<std::unique_ptr<TimerCore>, 12> mTimers;
  std::array<std::unique_ptr<AudioChannel>, 4> mAudioChannels;
  std::array<uint8_t, 32> mPalette;
  std::array<uint8_t, 4> mAttenuation;
  std::array<int16_t, 4> mAttenuationLeft;
  std::array<int16_t, 4> mAttenuationRight;

  std::unique_ptr<DisplayGenerator> mDisplayGenerator;

  ParallelPort mParallelPort;

  struct DisplayRegs
  {
    uint16_t dispAdr;
    bool dispColor;
    bool dispFourBit;
    bool dispFlip;
    bool DMAEnable;
  } mDisplayRegs;


  bool mSuzyDone;

  uint8_t mPan;
  uint8_t mStereo;
  uint8_t mSerDat;
  uint8_t mIRQ;
};

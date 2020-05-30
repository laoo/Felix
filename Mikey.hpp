#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <functional>
#include <utility>
#include "ActionQueue.hpp"
#include "DisplayGenerator.hpp"
#include "ParallelPort.hpp"

class BusMaster;
class TimerCore;
class AudioChannel;
class DisplayGenerator;

class Mikey
{
public:
  Mikey( BusMaster & busMaster, std::function<void( DisplayGenerator::Pixel const* )> const& fun );
  ~Mikey();

  struct WriteAction
  {
    enum class Type
    {
      NONE,
      ENQUEUE_ACTION,
      START_SUZY,
    } type;

    SequencedAction action;

    operator Type()
    {
      return type;
    }
  };

  uint64_t requestAccess( uint64_t tick, uint16_t address );
  uint8_t read( uint16_t address );
  WriteAction write( uint16_t address, uint8_t value );
  SequencedAction fireTimer( uint64_t tick, uint32_t timer );
  void setDMAData( uint64_t tick, uint64_t data );
  uint8_t getIRQ() const;
  void suzyDone();
  std::pair<float, float> sampleAudio() const;


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

  static constexpr uint16_t MPAN         = 0x44;
  static constexpr uint16_t MSTEREO      = 0x50;
  static constexpr uint16_t INTRST       = 0x80;
  static constexpr uint16_t INTSET       = 0x81;
  static constexpr uint16_t SYSCTL1      = 0x87;
  static constexpr uint16_t MIKEYHREV    = 0x89;
  static constexpr uint16_t IODIR        = 0x8a;
  static constexpr uint16_t IODAT        = 0x8b;
  static constexpr uint16_t SERCTL       = 0x8c;
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

  struct SERCTL
  {
    //write
    static constexpr uint8_t TXINTEN  = 0x80;   //transmitter interrupt enable
    static constexpr uint8_t RXINTEN  = 0x40;   //receive interrupt enable
    static constexpr uint8_t PAREN    = 0x10;   //xmit parity enable( if 0, PAREVEN is the bit sent )
    static constexpr uint8_t RESETERR = 0x08;   //reset all errors
    static constexpr uint8_t TXOPEN   = 0x04;   //1 open collector driver, 0 = TTL driver
    static constexpr uint8_t TXBRK    = 0x02;   //send a break ( for as long as the bit is set )
    static constexpr uint8_t PAREVEN  = 0x01;   //send / rcv even parity

    //read
    static constexpr uint8_t TXRDY    = 0x80; //transmitter buffer empty
    static constexpr uint8_t RXRDY    = 0x80; //receive character ready
    static constexpr uint8_t TXEMPTY  = 0x80; //transmitter totaiy done
    static constexpr uint8_t PARERR   = 0x80; //received parity error
    static constexpr uint8_t OVERRUN  = 0x80; //received overrun error
    static constexpr uint8_t FRAMERR  = 0x80; //received framing error
    static constexpr uint8_t RXBRK    = 0x80; //break recieved( 24 bit periods )
    static constexpr uint8_t PARBIT   = 0x80; //9th bit
  };

private:
  BusMaster & mBusMaster;
  uint64_t mAccessTick;

  std::array<std::unique_ptr<TimerCore>, 12> mTimers;
  std::array<std::unique_ptr<AudioChannel>, 4> mAudioChannels;
  std::array<uint8_t, 32> mPalette;

  std::unique_ptr<DisplayGenerator> mDisplayGenerator;

  ParallelPort mParallelPort;

  struct DisplayRegs
  {
    uint16_t dispAdr;
    bool dispColor;
    bool dispFourBit;
    bool dispFlip;
    bool DMAEnable;
    uint8_t pbkup;
  } mDisplayRegs;

  struct SerCtl
  {
    bool txinten;
    bool rxinten;
    bool paren;
    bool txopen;
    bool txbrk;
    bool pareven;
    bool txrdy;
    bool rxrdy;
    bool txempty;
    bool parerr;
    bool overrun;
    bool framerr;
    bool rxbrk;
    bool parbit;
  } mSerCtl;

  bool mSuzyDone;

  uint8_t mSerDat;
  uint8_t mIRQ;
};

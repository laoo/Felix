#pragma once

class ComLynx
{
public:

  ComLynx();

  bool pulse();
  void setCtrl( uint8_t data );
  void setData( uint8_t data );
  uint8_t getCtrl() const;
  uint8_t getData();
  bool present() const;

private:

  void setRead( uint8_t shift );

  struct SERCTL
  {
    //write
    static constexpr uint8_t TXINTEN = 0x80;   //transmitter interrupt enable
    static constexpr uint8_t RXINTEN = 0x40;   //receive interrupt enable
    static constexpr uint8_t PAREN = 0x10;   //xmit parity enable( if 0, PAREVEN is the bit sent )
    static constexpr uint8_t RESETERR = 0x08;   //reset all errors
    static constexpr uint8_t TXOPEN = 0x04;   //1 open collector driver, 0 = TTL driver
    static constexpr uint8_t TXBRK = 0x02;   //send a break ( for as long as the bit is set )
    static constexpr uint8_t PAREVEN = 0x01;   //send / rcv even parity

    //read
    static constexpr uint8_t TXRDY = 0x80; //transmitter buffer empty
    static constexpr uint8_t RXRDY = 0x40; //receive character ready
    static constexpr uint8_t TXEMPTY = 0x20; //transmitter totaiy done
    static constexpr uint8_t PARERR = 0x10; //received parity error
    static constexpr uint8_t OVERRUN = 0x08; //received overrun error
    static constexpr uint8_t FRAMERR = 0x04; //received framing error
    static constexpr uint8_t RXBRK = 0x02; //break recieved( 24 bit periods )
    static constexpr uint8_t PARBIT = 0x01; //9th bit
  };

  uint8_t mWriteCtrl;
  uint8_t mReadCtrl;
  uint8_t mHold;
  uint8_t mShift;
  uint8_t mRead;
  int mCycle;

};

#pragma once

class ComLynxWire;

class ComLynx
{
public:

  ComLynx( std::shared_ptr<ComLynxWire> comLynxWire );
  ~ComLynx();

  bool present() const;
  bool pulse();
  void setCtrl( uint8_t ctrl );
  void setData( uint8_t data );
  uint8_t getCtrl() const;
  uint8_t getData();

  bool interrupt() const;

private:

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

  int mId;

  class Transmitter
  {
  public:
    Transmitter( int id, std::shared_ptr<ComLynxWire> comLynxWire );

    void setCtrl( uint8_t ctrl );
    void setData( int data );
    uint8_t getStatus() const;
    bool interrupt() const;
    void process();

  private:

    void pull( int bit );

    std::shared_ptr<ComLynxWire> mWire;
    std::optional<int> mData;
    int mState;
    int mCounter;
    int mParity;
    int mShifter;
    int mParEn;
    int mIntEn;
    int mTxBrk;
    int mParBit;
    int mId;
  } mTx;

  class Receiver
  {
  public:
    Receiver( int id, std::shared_ptr<ComLynxWire> comLynxWire );

    void setCtrl( uint8_t ctrl );
    int getData();
    uint8_t getStatus() const;
    bool interrupt() const;
    void process();

  private:
    std::shared_ptr<ComLynxWire> mWire;
    std::optional<int> mData;
    int mCounter;
    int mParity;
    int mShifter;
    int mParErr;
    int mFrameErr;
    int mRxBrk;
    int mOverrun;
    int mIntEn;
    int mId;
  } mRx;

};

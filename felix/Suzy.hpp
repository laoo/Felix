#pragma once
#include "ActionQueue.hpp"
#include "KeyInput.hpp"
#include "SuzyMath.hpp"

class Felix;

class ISuzyProcess
{
public:

  struct Request
  {
    uint32_t mask;
    uint16_t addr;
    uint8_t value;
    enum Type
    {
      FINISH,
      READ,
      READ4,
      WRITE,
      COLRMW,
      VIDRMW,
      XOR
    } type;

    Request( Type type = FINISH, uint16_t addr = 0, uint8_t value = 0, uint32_t mask = 0 ) : mask{ mask }, addr{ addr }, value{ value }, type{ type } {}
  };


public:

  virtual ~ISuzyProcess() = default;
  virtual Request const* advance() = 0;
  virtual void respond( uint32_t value ) = 0;

  struct RequestFinish : public Request
  {
    RequestFinish() : Request{ FINISH } {}
  };
  struct RequestRead : public Request
  {
    RequestRead( uint16_t addr ) : Request{ READ, addr } {}
  };
  struct RequestRead4 : public Request
  {
    RequestRead4( uint16_t addr ) : Request{ READ4, addr } {}
  };
  struct RequestWrite : public Request
  {
    RequestWrite( uint16_t addr, uint8_t value ) : Request{ WRITE, addr, value } {}
  };
  struct RequestColRMW : public Request
  {
    RequestColRMW( uint16_t addr, uint32_t mask, uint8_t value ) : Request{ COLRMW, addr, value, mask } {}
  };
  struct RequestVidRMW : public Request
  {
    RequestVidRMW( uint16_t addr, uint8_t value, uint8_t mask ) : Request{ VIDRMW, addr, value, mask } {}
  };
  struct RequestXOR : public Request
  {
    RequestXOR( uint16_t addr, uint8_t value ) : Request{ XOR, addr, value } {}
  };
};

class Suzy
{
public:
  Suzy( Felix & felix, std::function<KeyInput()> const& inputProvider );

  uint64_t requestAccess( uint64_t tick, uint16_t address );
  uint8_t read( uint16_t address );
  void write( uint16_t address, uint8_t value );

  std::shared_ptr<ISuzyProcess> suzyProcess();

  friend class SuzyProcess;

  static constexpr uint16_t TMPADR    = 0x00;
  static constexpr uint16_t TILTACUM  = 0x02;
  static constexpr uint16_t HOFF      = 0x04;
  static constexpr uint16_t VOFF      = 0x06;
  static constexpr uint16_t VIDBAS    = 0x08;
  static constexpr uint16_t COLLBAS   = 0x0a;
  static constexpr uint16_t VIDADR    = 0x0c;
  static constexpr uint16_t COLLADR   = 0x0e;
  static constexpr uint16_t SCBNEXT   = 0x10;
  static constexpr uint16_t SPRDLINE  = 0x12;
  static constexpr uint16_t HPOSSTRT  = 0x14;
  static constexpr uint16_t VPOSSTRT  = 0x16;
  static constexpr uint16_t SPRHSIZ   = 0x18;
  static constexpr uint16_t SPRVSIZ   = 0x1a;
  static constexpr uint16_t STRETCH   = 0x1c;
  static constexpr uint16_t TILT      = 0x1e;
  static constexpr uint16_t SPRDOFF   = 0x20;
  static constexpr uint16_t SCVPOS    = 0x22;
  static constexpr uint16_t COLLOFF   = 0x24;
  static constexpr uint16_t VSIZACUM  = 0x26;
  static constexpr uint16_t HSIZOFF   = 0x28;
  static constexpr uint16_t VSIZOFF   = 0x2a;
  static constexpr uint16_t SCBADR    = 0x2c;
  static constexpr uint16_t PROCADR   = 0x2e;

  static constexpr uint16_t MATHD     = 0x52;
  static constexpr uint16_t MATHC     = 0x53;
  static constexpr uint16_t MATHB     = 0x54;
  static constexpr uint16_t MATHA     = 0x55;
  static constexpr uint16_t MATHP     = 0x56;
  static constexpr uint16_t MATHN     = 0x57;

  static constexpr uint16_t MATHH     = 0x60;
  static constexpr uint16_t MATHG     = 0x61;
  static constexpr uint16_t MATHF     = 0x62;
  static constexpr uint16_t MATHE     = 0x63;

  static constexpr uint16_t MATHM     = 0x6c;
  static constexpr uint16_t MATHL     = 0x6d;
  static constexpr uint16_t MATHK     = 0x6e;
  static constexpr uint16_t MATHJ     = 0x6f;

  static constexpr uint16_t SPRCTL0   = 0x80;
  static constexpr uint16_t SPRCTL1   = 0x81;
  static constexpr uint16_t SPRCOLL   = 0x82;
  static constexpr uint16_t SPRINIT   = 0x83;
  static constexpr uint16_t SUZYHREV  = 0x88;
  static constexpr uint16_t SUZYSREV  = 0x89;
  static constexpr uint16_t SUZYBUSEN = 0x90;
  static constexpr uint16_t SPRGO     = 0x91;
  static constexpr uint16_t SPRSYS    = 0x92;
  static constexpr uint16_t JOYSTICK  = 0xb0;
  static constexpr uint16_t SWITCHES  = 0xb1;
  static constexpr uint16_t RCART0    = 0xb2;
  static constexpr uint16_t RCART1    = 0xb3;

  struct SPRCTL0
  {
    static constexpr uint8_t BITS_MASK         = 0b11000000;
    static constexpr uint8_t ONE_PER_PIXEL     = 0b00000000;
    static constexpr uint8_t TWO_PER_PIXEL     = 0b01000000;
    static constexpr uint8_t THREE_PER_PIXEL   = 0b10000000;
    static constexpr uint8_t FOUR_PER_PIXEL    = 0b11000000;
    static constexpr uint8_t HFLIP             = 0b00100000;
    static constexpr uint8_t VFLIP             = 0b00010000;
    static constexpr uint8_t SPRITE_MASK       = 0b00000111;
    static constexpr uint8_t SHADOW_SPRITE     = 0b00000111;
    static constexpr uint8_t XOR_SPRITE        = 0b00000110;
    static constexpr uint8_t XOR_SHADOW_SPRITE = 0b00000110;
    static constexpr uint8_t NONCOLL_SPRITE    = 0b00000101;
    static constexpr uint8_t NORMAL_SPRITE     = 0b00000100;
    static constexpr uint8_t BOUNDARY_SPRITE   = 0b00000011;
    static constexpr uint8_t BSHADOW_SPRITE    = 0b00000010;
    static constexpr uint8_t BACKNONCOLL_SPRITE= 0b00000001;
    static constexpr uint8_t BACKGROUND_SPRITE = 0b00000000;
    static constexpr uint8_t BACK_SHADOW_SPRITE= 0b00000000;
  };

  struct SPRCTL1
  {
    static constexpr uint8_t LITERAL           = 0b10000000;
    static constexpr uint8_t ALGO_3            = 0b01000000; //broken, do not set this bit!
    static constexpr uint8_t RELOAD_MASK       = 0b00110000;
    static constexpr uint8_t RELOAD_NONE       = 0b00000000; //Reload nothing
    static constexpr uint8_t RELOAD_HV         = 0b00010000; //Reload hsize, vsize
    static constexpr uint8_t RELOAD_HVS        = 0b00100000; //Reload hsize, vsize, stretch
    static constexpr uint8_t RELOAD_HVST       = 0b00110000; //Reload hsize, vsize, stretch, tilt
    static constexpr uint8_t REUSE_PALETTE     = 0b00001000;
    static constexpr uint8_t SKIP_SPRITE       = 0b00000100;
    static constexpr uint8_t STARGING_QUAD_MASK= 0b00000011;
    static constexpr uint8_t DRAW_UP           = 0b00000010;
    static constexpr uint8_t DRAW_LEFT         = 0b00000001;
  };

  struct SPRCOLL
  {
    static constexpr uint8_t NO_COLLIDE        = 0b00100000;
    static constexpr uint8_t NUMBER_MASK       = 0b00001111;
  };

  struct SUZYBUSEN
  {
    static constexpr uint8_t ENABLE   = 0b00000001; //Suzy Bus Enable, 0 = disabled
  };

  struct SPRGO
  {
    static constexpr uint8_t EVER_ON           = 0b00000100; //enable everon detector : 1 = enabled.
    static constexpr uint8_t SPRITE_GO         = 0b00000001; //Sprite process enabled : 0 = disabled.Write a 1 to start the process, at completion of process this bit will be reset to 0. Either setting or clearing this bit will clear the Stop At End Of Current Sprite bit.
  };

  struct SPRSYS
  {
    //These are the SPRSYS flag definitions when writing
    static constexpr uint8_t SIGNMATH          = 0b10000000; //Signmath: 0 = unsigned math, 1 = signed math.
    static constexpr uint8_t ACCUMULATE        = 0b01000000; //OK to accumvlate : 0 = do not accumulate, 1 = yes, accumulate.
    static constexpr uint8_t NO_COLLIDE        = 0b00100000; //dont collide : 1 = dont collide with any sprites.
    static constexpr uint8_t VSTRETCH          = 0b00010000; //Vstretch: 1 = stretch the v, 0 = Don't play with it, it will grow by itself.
    static constexpr uint8_t LEFTHAND          = 0b00001000; //Lefthand: 0 = normal handed
    static constexpr uint8_t UNSAFEACCESSRST   = 0b00000100; //Clear the 'unsafeAccess' bit: 1 = clear it, 0 = no change.
    static constexpr uint8_t SPRITESTOP        = 0b00000010; //Stop at end of current sprite : 1 = request to stop.Continue sprite processing by setting the Sprite Process Start Bit.Either setting or clearing the SPSB will clear this stop request.
    //These are the SPRSYS flag definitions when reading
    static constexpr uint8_t MATHWORKING       = 0b10000000; //Math in process
    static constexpr uint8_t MATHWARNING       = 0b01000000; //Mathbit: If mult, 1 = accumulator overflow.If div, 1 = div by zero attempted.
    static constexpr uint8_t MATHCARRY         = 0b00100000; //Last carry bit.
    static constexpr uint8_t VSTRETCHING       = 0b00010000; //Vstretch
    static constexpr uint8_t LEFTHANDED        = 0b00001000; //Lefthand
    static constexpr uint8_t UNSAFEACCESS      = 0b00000100; //UnsafeAccess: 1 = Unsafe Access was performed.
    static constexpr uint8_t SPRITETOSTOP      = 0b00000010; //Stop at end of current sprite : 1 = request to stop.
    static constexpr uint8_t SPRITEWORKING     = 0b00000001; //Sprite process was started and has neither completed nor been stopped.
  };

  struct JOYSTICK
  {
    static constexpr uint8_t DOWN              = 0b10000000;
    static constexpr uint8_t UP                = 0b01000000;
    static constexpr uint8_t RIGHT             = 0b00100000;
    static constexpr uint8_t LEFT              = 0b00010000;
    static constexpr uint8_t OPTION1           = 0b00001000;
    static constexpr uint8_t OPTION2           = 0b00000100;
    static constexpr uint8_t INNER             = 0b00000010;
    static constexpr uint8_t OUTER             = 0b00000001;
    static constexpr uint8_t A                 = OUTER;
    static constexpr uint8_t B                 = INNER;
    static constexpr uint8_t RESTART           = OPTION1;
    static constexpr uint8_t FLIP              = OPTION2;
  };

  struct SWITCHES
  {
    static constexpr uint8_t CART1_STROBE = 0b00000100;
    static constexpr uint8_t CART0_STROBE = 0b00000010;
    static constexpr uint8_t PAUSE_SWITCH = 0b00000001;
  };

  struct Reg
  {
    union
    {
      uint16_t w;
      struct
      {
        uint8_t l;
        uint8_t h;
      };
    };

    Reg( uint16_t w = 0 ) : w{ w } {}

    Reg & operator=( uint16_t value ) { w = value; return *this; }
    operator uint16_t() const { return w; }
    Reg operator++( int )
    {
      Reg result{ w++ };
      return result;
    }
    Reg & operator+=( int offset )
    {
      w += offset;
      return *this;
    }
  };

  enum class BPP
  {
    ONE     = 0b00000000,
    TWO     = 0b01000000,
    THREE   = 0b10000000,
    FOUR    = 0b11000000
  };

  enum class Sprite
  {
    SHADOW     = 0b00000111,
    XOR        = 0b00000110,
    XOR_SHADOW = 0b00000110,
    NONCOLL    = 0b00000101,
    NORMAL     = 0b00000100,
    BOUNDARY   = 0b00000011,
    BSHADOW    = 0b00000010,
    BACKNONCOLL= 0b00000001,
    BACKGROUND = 0b00000000,
    BACK_SHADOW= 0b00000000
  };

  enum class Reload
  {
    NONE       = 0b00000000, //Reload nothing
    HV         = 0b00010000, //Reload hsize, vsize
    HVS        = 0b00100000, //Reload hsize, vsize, stretch
    HVST       = 0b00110000 //Reload hsize, vsize, stretch, tilt
  };

  enum class Quadrant
  {
    DOWN_RIGHT    = 0b00,
    DOWN_LEFT     = 0b01,
    UP_RIGHT      = 0b10,
    UP_LEFT       = 0b11
  };

private:
  void writeSPRCTL0( uint8_t value );
  void writeSPRCTL1( uint8_t value );
  void writeSPRCOLL( uint8_t value );
  void writeCart( int number, uint8_t value );
  int bpp() const;

private:
  Felix & mFelix;
  struct SCB
  {
    Reg tmpadr;
    Reg tiltacum;
    Reg hoff;
    Reg voff;
    Reg vidbas;
    Reg collbas;
    Reg vidadr;
    Reg colladr;
    Reg scbnext;
    Reg sprdline;
    Reg hposstrt;
    Reg vposstrt;
    Reg sprhsiz;
    Reg sprvsiz;
    Reg stretch;
    Reg tilt;
    Reg sprdoff;
    Reg sprvpos;
    Reg colloff;
    Reg vsizacum;
    Reg hsizoff;
    Reg vsizoff;
    Reg scbadr;
    Reg procadr;
  } mSCB;

  SuzyMath mMath;
  std::function<KeyInput()> const mInputProvider;
  uint64_t mAccessTick;

  std::array<uint8_t, 16> mPalette;
  bool mBusEnable;          //Suzy Bus Enable, 0 = disabled
  bool mNoCollide;          //dont collide : 1 = dont collide with any sprites.
  bool mVStretch;           //Vstretch: 1 = stretch the v, 0 = Don't play with it, it will grow by itself.
  bool mLeftHand;           //Lefthand: 0 = normal handed
  bool mUnsafeAccess;       //Clear the 'unsafeAccess' bit: 1 = clear it, 0 = no change.
  bool mSpriteStop;        //Stop at end of current sprite : 1 = request to stop.Continue sprite processing by setting the Sprite Process Start Bit.Either setting or clearing the SPSB will clear this stop request.
  bool mSpriteWorking;     //Sprite process was started and has neither completed nor been stopped.
  bool mHFlip;
  bool mVFlip;
  bool mLiteral;
  bool mAlgo3;  //broken, do not set this bit!
  bool mReusePalette;
  bool mSkipSprite;
  bool mEveron;
  bool mDisableCollisions;
  Quadrant mStartingQuadrant;
  BPP mBpp;
  Sprite mSpriteType;
  Reload mReload;
  uint8_t mSprColl;
  uint8_t mSprInit; //should be 0xf3
  std::optional<uint8_t> mFred;

  static constexpr std::array<std::array<Quadrant, 4>,4> mQuadrantOrder ={
    std::array<Quadrant, 4>{ Quadrant::DOWN_RIGHT, Quadrant::UP_RIGHT, Quadrant::UP_LEFT, Quadrant::DOWN_LEFT },
    std::array<Quadrant, 4>{ Quadrant::DOWN_LEFT, Quadrant::DOWN_RIGHT, Quadrant::UP_RIGHT, Quadrant::UP_LEFT },
    std::array<Quadrant, 4>{ Quadrant::UP_RIGHT, Quadrant::UP_LEFT, Quadrant::DOWN_LEFT, Quadrant::DOWN_RIGHT },
    std::array<Quadrant, 4>{ Quadrant::UP_LEFT, Quadrant::DOWN_LEFT, Quadrant::DOWN_RIGHT, Quadrant::UP_RIGHT }
  };

  static constexpr int mScreenWidth = 160;
  static constexpr int mScreenHeight = 102;
};




#pragma once
#include <cstdint>
#include <experimental/coroutine>

class BusMaster;

//stolen form http://www.maizure.org/projects/decoded-bisqwit-nes-emulator/nesemu1_lines.txt
template< unsigned bitno, typename T = uint8_t>
struct RegBit
{
  T data;
  template < typename T2>
  RegBit & operator=( T2 val )
  {
    data = ( data & ~( 1 << bitno ) ) | ( ( val & 1 ) << bitno );
    return *this;
  }
  operator unsigned() const
  {
    return ( data >> bitno ) & 1;
  }

  explicit operator bool () const
  {
    return ( ( data >> bitno ) & 1 ) != 0;
  }

};

enum class Opcode : uint8_t;

struct CPU
{
  uint8_t a;
  uint8_t x;
  uint8_t y;
  union
  {
    uint16_t s;
    struct
    {
      uint8_t sl;
      uint8_t sh;
    };
  };
  union
  {
    uint8_t P;
    RegBit<0> C;
    RegBit<1> Z;
    RegBit<2> I;
    RegBit<3> D;
    RegBit<6> V;
    RegBit<7> N;
  };
  union
  {
    uint16_t pc;
    struct
    {
      uint8_t pcl;
      uint8_t pch;
    };
  };
  int interrupt;

  static const int I_NONE  = 0;
  static const int I_IRQ = 1;
  static const int I_NMI = 2;
  static const int I_RESET = 4;

  CPU() : a{}, x{}, y{}, s{ 0x1ff }, pc{}, interrupt{ I_RESET }, P{}
  {
  }

  uint8_t p() const
  {
    return P | 0x20;
  }

  uint8_t pbrk() const
  {
    return P | 0x30;
  }

  void setnz( uint8_t v )
  {
    Z = v == 0 ? 1 : 0;
    N = v >= 80 ? 1 : 0;
  }

  void setz( uint8_t v )
  {
    Z = v == 0 ? 1 : 0;
  }

  void asl( uint8_t & val );
  void lsr( uint8_t & val );
  void rol( uint8_t & val );
  void ror( uint8_t & val );
  bool executeR( Opcode opcode, uint8_t value );


private:

};

struct Read
{
  uint16_t address;

  Read( uint16_t a ) : address{ a } {}
};

struct Write
{
  uint16_t address;
  uint8_t value;

  Write( uint16_t a, uint8_t v ) : address{ a }, value{ v } {}
};


struct CPURequest
{
  enum class Type
  {
    NONE,
    READ,
    WRITE,
  } mType;

  uint16_t address;
  uint8_t value;

  CPURequest() : mType{ Type::NONE }, address{}, value{} {}
  CPURequest( Read r ) : mType{ Type::READ }, address{ r.address }, value{} {}
  CPURequest( Write w ) : mType{ Type::WRITE }, address{ w.address }, value{ w.value } {}

  void resume()
  {
    mType = Type::NONE;
    coro.resume();
  }

  std::experimental::coroutine_handle<> coro;
};

struct AwaitRead
{
  CPURequest * req;

  bool await_ready()
  {
    return false;
  }

  int await_resume()
  {
    return req->value;
  }

  void await_suspend( std::experimental::coroutine_handle<> c )
  {
    req->coro = c;
  }
};

struct AwaitWrite
{
  CPURequest * req;

  bool await_ready()
  {
    return false;
  }

  void await_resume()
  {
  }

  void await_suspend( std::experimental::coroutine_handle<> c )
  {
    req->coro = c;
  }
};

struct CpuLoop
{
  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;

  struct promise_type
  {
    auto get_return_object() { return CpuLoop{ *this, handle::from_promise( *this ) }; }
    auto initial_suspend() { return std::experimental::suspend_always{}; }
    auto final_suspend() noexcept { return std::experimental::suspend_always{}; }
    void return_void() {}
    void unhandled_exception() { std::terminate(); }
    AwaitRead yield_value( Read r );
    AwaitWrite yield_value( Write r );

    BusMaster * mBus;
  };

  CpuLoop( promise_type & promise, handle c ) : promise{ promise }, coro{ c }
  {
  }

  ~CpuLoop()
  {
    if ( coro )
      coro.destroy();
  }

  void setBusMaster( BusMaster * bus )
  {
    promise.mBus = bus;
    coro.resume();
  }

  handle coro;
  promise_type & promise;
};

CpuLoop cpuLoop( CPU & cpu );

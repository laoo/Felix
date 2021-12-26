#pragma once

#include "Opcodes.hpp"

struct CPUState
{
  static constexpr int I_NONE = 0;
  static constexpr int I_IRQ = 1;
  static constexpr int I_NMI = 2;
  static constexpr int I_RESET = 4;

  static constexpr int bitC = 0b00000001;
  static constexpr int bitZ = 0b00000010;
  static constexpr int bitI = 0b00000100;
  static constexpr int bitD = 0b00001000;
  static constexpr int bitB = 0b00010000;
  static constexpr int bit1 = 0b00100000;
  static constexpr int bitV = 0b01000000;
  static constexpr int bitN = 0b10000000;

  template<int SHIFT, uint8_t V>
  struct Flag
  {
    uint8_t value;

    explicit operator bool() const
    {
      return value == V;
    }

    uint8_t bit() const
    {
      return value == V ? SHIFT : 0;
    }

    void set()
    {
      value = V;
    }

    void clear()
    {
      value = '-';
    }

    void set( bool v )
    {
      value = v ? V : '-';
    }
  };

  union
  {
    uint64_t p_;
    struct
    {
      Flag<bitN, 'N'> n;
      Flag<bitV, 'V'> v;
      Flag<bitD, 'D'> d;
      Flag<bitI, 'I'> i;
      Flag<bitZ, 'Z'> z;
      Flag<bitC, 'C'> c;
      uint8_t padding;
      uint8_t interrupt;
    };
  };


  void printP( char* out ) const
  {
    *(uint64_t*)out = p_;
    //clearing interrupt field
    out[7] = ' ';
  }

  static CPUState reset()
  {
    CPUState result;
    result.n.clear();
    result.v.clear();
    result.d.clear();
    result.i.clear();
    result.z.clear();
    result.c.clear();
    result.padding = ' ';
    result.interrupt = (uint8_t)I_RESET;
    result.pc = 0;
    result.s = 0x1ff;
    result.op = Opcode::BRK_BRK;
    result.a = 0;
    result.x = 0;
    result.y = 0;
    result.ea = 0;
    result.fa = 0;
    result.t = 0;
    result.m1 = 0;
    result.m2 = 0;

    return result;
  }

  /*
    https://github.com/spacerace/6502/blob/master/doc/6502-asm-doc/the%20B%20flag%20and%20BRK%20instruction.txt:
    No actual "B" flag exists inside the 6502's processor status register. The B
    flag only exists in the status flag byte pushed to the stack. Naturally,
    when the flags are restored (via PLP or RTI), the B bit is discarded.

    Depending on the means, the B status flag will be pushed to the stack as
    either 0 or 1.

    software instructions BRK & PHP will push the B flag as being 1.
    hardware interrupts IRQ & NMI will push the B flag as being 0.
  */
  bool getB() const
  {
    return interrupt == 0;
  }

  bool get1() const
  {
    return true;
  }

  uint8_t getP() const
  {
    return
      ( c ? bitC : 0 ) |
      ( z ? bitZ : 0 ) |
      ( i ? bitI : 0 ) |
      ( d ? bitD : 0 ) |
      ( getB() ? bitB : 0 ) |
      ( bit1 ) |
      ( v ? bitV : 0 ) |
      ( n ? bitN : 0 );
  }

  void setP( uint8_t value )
  {
    c.set( ( value & bitC ) != 0 );
    z.set( ( value & bitZ ) != 0 );
    i.set( ( value & bitI ) != 0 );
    d.set( ( value & bitD ) != 0 );
    v.set( ( value & bitV ) != 0 );
    n.set( ( value & bitN ) != 0 );
  }

  void setnz( uint8_t v )
  {
    n.set( v >= 0x80 );
    z.set( v == 0 );
  }

  void setz( uint8_t v )
  {
    z.set( v == 0 );
  }

  union
  {
    uint16_t pc;
    struct
    {
      uint8_t pcl;
      uint8_t pch;
    };
  };
  union
  {
    uint16_t s;
    struct
    {
      uint8_t sl;
      uint8_t sh;
    };
  };

  Opcode op;

  uint8_t a;
  uint8_t x;
  uint8_t y;

  union
  {
    uint16_t ea{};
    struct
    {
      uint8_t eal;
      uint8_t eah;
    };
  };

  union
  {
    uint16_t fa{};
    struct
    {
      uint8_t fal;
      uint8_t fah;
    };
  };

  union
  {
    uint16_t t;
    struct
    {
      uint8_t tl;
      uint8_t th;
    };
  };

  uint8_t m1;
  uint8_t m2;

};

#pragma once

#include "IMemoryAccessTrap.hpp"
#include "generator.hpp"

class Core;
class ScriptDebuggerEscapes;

class ScriptDebugger
{
  class CompositeTrap : public IMemoryAccessTrap
  {
    std::shared_ptr<IMemoryAccessTrap> mT1;
    std::shared_ptr<IMemoryAccessTrap> mT2;
  public:
    CompositeTrap( std::shared_ptr<IMemoryAccessTrap> t1, std::shared_ptr<IMemoryAccessTrap> t2 ) : mT1{ std::move( t1 ) }, mT2{ std::move( t2 ) } {}
    ~CompositeTrap() override = default;

    uint8_t trap( Core& core, uint16_t address, uint8_t orgValue ) override
    {
      auto temp = mT1->trap( core, address, orgValue );
      return mT2->trap( core, address, temp );
    }

    Kind getKind() const override
    {
      return mT1->getKind() | mT2->getKind();
    }
  };

public:

  enum class Type : uint16_t
  {
    RAM_EXECUTE,
    RAM_READ,
    RAM_WRITE,
    ROM_READ,
    ROM_WRITE,
    ROM_EXECUTE,
    MIKEY_READ,
    MIKEY_WRITE,
    SUZY_READ,
    SUZY_WRITE,
    MAPCTL_READ,
    MAPCTL_WRITE
  };

  ScriptDebugger() = default;
  ~ScriptDebugger() = default;

  cppcoro::generator<std::tuple<Type, uint16_t, std::shared_ptr<IMemoryAccessTrap>>> getTraps( IMemoryAccessTrap::Kind kind )
  {
    for ( int i = 0; i < 0xffff; ++i )
    {
      if ( mRamReadTraps[i] && mRamReadTraps[i]->getKind() == kind )
      {
        co_yield std::tuple<Type, uint16_t, std::shared_ptr<IMemoryAccessTrap>>( Type::RAM_READ, i, mRamReadTraps[i] );
      }
      if ( mRamWriteTraps[i] && mRamWriteTraps[i]->getKind() == kind )
      {
        co_yield std::tuple<Type, uint16_t, std::shared_ptr<IMemoryAccessTrap>>( Type::RAM_WRITE, i, mRamWriteTraps[i] );
      }
      if ( mRamExecuteTraps[i] && mRamExecuteTraps[i]->getKind() == kind )
      {
        co_yield std::tuple<Type, uint16_t, std::shared_ptr<IMemoryAccessTrap>>( Type::RAM_EXECUTE, i, mRamExecuteTraps[i] );
      }
    }

    for ( int i = 0; i < 0x200; ++i )
    {
      if ( mRomReadTraps[i] && mRomReadTraps[i]->getKind() == kind )
      {
        co_yield std::tuple<Type, uint16_t, std::shared_ptr<IMemoryAccessTrap>>( Type::ROM_READ, i, mRomReadTraps[i] );
      }
      if ( mRomWriteTraps[i] && mRomWriteTraps[i]->getKind() == kind )
      {
        co_yield std::tuple<Type, uint16_t, std::shared_ptr<IMemoryAccessTrap>>( Type::ROM_WRITE, i, mRomWriteTraps[i] );
      }
      if ( mRomExecuteTraps[i] && mRomExecuteTraps[i]->getKind() == kind )
      {
        co_yield std::tuple<Type, uint16_t, std::shared_ptr<IMemoryAccessTrap>>( Type::ROM_EXECUTE, i, mRomExecuteTraps[i] );
      }
    }

    for ( int i = 0; i < 0x100; ++i )
    {
      if ( mSuzyReadTraps[i] && mSuzyReadTraps[i]->getKind() == kind )
      {
        co_yield std::tuple<Type, uint16_t, std::shared_ptr<IMemoryAccessTrap>>( Type::SUZY_READ, i, mSuzyReadTraps[i] );
      }
      if ( mSuzyWriteTraps[i] && mSuzyWriteTraps[i]->getKind() == kind )
      {
        co_yield std::tuple<Type, uint16_t, std::shared_ptr<IMemoryAccessTrap>>( Type::SUZY_WRITE, i, mSuzyWriteTraps[i] );
      }

      if ( mMikeyReadTraps[i] && mMikeyReadTraps[i]->getKind() == kind )
      {
        co_yield std::tuple<Type, uint16_t, std::shared_ptr<IMemoryAccessTrap>>( Type::MIKEY_READ, i, mMikeyReadTraps[i] );
      }
      if ( mMikeyWriteTraps[i] && mMikeyWriteTraps[i]->getKind() == kind )
      {
        co_yield std::tuple<Type, uint16_t, std::shared_ptr<IMemoryAccessTrap>>( Type::MIKEY_WRITE, i, mMikeyWriteTraps[i] );
      }
    }
  }

  void deleteTrap( Type type, uint16_t address )
  {
    switch ( type )
    {
    case Type::RAM_READ:
      mRamReadMask[address] = 0;
      mRamReadTraps[address] = nullptr;
      break;
    case Type::RAM_WRITE:
      mRamWriteMask[address] = 0;
      mRamWriteTraps[address] = nullptr;
      break;
    case Type::RAM_EXECUTE:
      mRamExecuteMask[address] = 0;
      mRamExecuteTraps[address] = nullptr;
      break;
    case Type::ROM_READ:
      mRomReadMask[address] = 0;
      mRomReadTraps[address] = nullptr;
      break;
    case Type::ROM_WRITE:
      mRomWriteMask[address] = 0;
      mRomWriteTraps[address] = nullptr;
      break;
    case Type::ROM_EXECUTE:
      mRomExecuteMask[address] = 0;
      mRomExecuteTraps[address] = nullptr;
      break;
    case Type::MIKEY_READ:
      mMikeyReadMask[address] = 0;
      mMikeyReadTraps[address] = nullptr;
      break;
    case Type::MIKEY_WRITE:
      mMikeyWriteMask[address] = 0;
      mMikeyWriteTraps[address] = nullptr;
      break;
    case Type::SUZY_READ:
      mSuzyReadMask[address] = 0;
      mSuzyReadTraps[address] = nullptr;
      break;
    case Type::SUZY_WRITE:
      mSuzyWriteMask[address] = 0;
      mSuzyWriteTraps[address] = nullptr;
      break;
    }
  }

  void addTrap( Type type, uint16_t address, std::shared_ptr<IMemoryAccessTrap> trap )
  {
    switch ( type )
    {
    case Type::RAM_READ:
      helper( { mRamReadTraps.data(), mRamReadTraps.size() }, mRamReadMask[address], address, std::move( trap ) );
      break;
    case Type::RAM_WRITE:
      helper( { mRamWriteTraps.data(), mRamWriteTraps.size() }, mRamWriteMask[address], address, std::move( trap ) );
      break;
    case Type::RAM_EXECUTE:
      helper( { mRamExecuteTraps.data(), mRamExecuteTraps.size() }, mRamExecuteMask[address], address, std::move( trap ) );
      break;
    case Type::ROM_READ:
      helper( { mRomReadTraps.data(), mRomReadTraps.size() }, mRomReadMask[address & 0x1ff], address & 0x1ff, std::move( trap ) );
      break;
    case Type::ROM_WRITE:
      helper( { mRomWriteTraps.data(), mRomWriteTraps.size() }, mRomWriteMask[address & 0x1ff], address & 0x1ff, std::move( trap ) );
      break;
    case Type::ROM_EXECUTE:
      helper( { mRomExecuteTraps.data(), mRomExecuteTraps.size() }, mRomExecuteMask[address & 0x1ff], address & 0x1ff, std::move( trap ) );
      break;
    case Type::MIKEY_READ:
      helper( { mMikeyReadTraps.data(), mMikeyReadTraps.size() }, mMikeyReadMask[address & 0xff], address & 0xff, std::move( trap ) );
      break;
    case Type::MIKEY_WRITE:
      helper( { mMikeyWriteTraps.data(), mMikeyWriteTraps.size() }, mMikeyWriteMask[address & 0xff], address & 0xff, std::move( trap ) );
      break;
    case Type::SUZY_READ:
      helper( { mSuzyReadTraps.data(), mSuzyReadTraps.size() }, mSuzyReadMask[address & 0xff], address & 0xff, std::move( trap ) );
      break;
    case Type::SUZY_WRITE:
      helper( { mSuzyWriteTraps.data(), mSuzyWriteTraps.size() }, mSuzyWriteMask[address & 0xff], address & 0xff, std::move( trap ) );
      break;
    case Type::MAPCTL_READ:
      helper( { &mMapCtlReadTrap, 1 }, 0, std::move( trap ) );
      break;
    case Type::MAPCTL_WRITE:
      helper( { &mMapCtlWriteTrap, 1 }, 0, std::move( trap ) );
      break;
    }
  }

  uint8_t readRAM( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( mRamReadMask( address ) )
    {
      return mRamReadTraps[address]->trap( core, address, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t writeRAM( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( mRamWriteMask( address ) )
    {
      return mRamWriteTraps[address]->trap( core, address, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t executeRAM( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( mRamExecuteMask( address ) )
    {
      return mRamExecuteTraps[address]->trap( core, address, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t readROM( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( mRomReadMask( address ) )
    {
      return mRomReadTraps[address]->trap( core, address + 0xfe00, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t writeROM( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( mRomWriteMask( address ) )
    {
      return mRomWriteTraps[address]->trap( core, address + 0xfe00, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t executeROM( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( mRomExecuteMask(address) )
    {
      return mRomExecuteTraps[address]->trap( core, address + 0xfe00, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t readMikey( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( mMikeyReadMask( address & 0xff ) )
    {
      return mMikeyReadTraps[address & 0xff]->trap( core, address, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t writeMikey( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( mMikeyWriteMask( address & 0xff ) )
    {
      return mMikeyWriteTraps[address & 0xff]->trap( core, address, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t readSuzy( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( mSuzyReadMask( address & 0xff ) )
    {
      return mSuzyReadTraps[address & 0xff]->trap( core, address, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t writeSuzy( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( mSuzyWriteMask( address & 0xff ) )
    {
      return mSuzyWriteTraps[address & 0xff]->trap( core, address, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t readMapCtl( Core& core, uint8_t orgValue )
  {
    if ( auto trap = mMapCtlReadTrap.get() )
    {
      return trap->trap( core, 0xfff9, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t writeMapCtl( Core& core, uint8_t orgValue )
  {
    if ( auto trap = mMapCtlWriteTrap.get() )
    {
      return trap->trap( core, 0xfff9, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

private:

  struct Proxy
  {
    uint8_t& byte;
    uint8_t mask;

    void operator=( bool bit )
    {
      if ( bit )
      {
        byte |= mask;
      }
      else
      {
        byte &= ~mask;
      }
    }

    explicit operator bool() const
    {
      return ( byte & mask ) != 0;
    }
  };

  template<size_t SIZE>
  struct BitArray : public std::array<uint8_t, SIZE / 8>
  {

    constexpr bool operator()( size_t pos ) const
    {
      return ( std::array<uint8_t, SIZE / 8>::operator[]( pos >> 3 ) & ( 1 << ( pos & 7 ) ) ) != 0;
    }

    constexpr Proxy operator[]( size_t pos )
    {
      return Proxy{ std::array<uint8_t, SIZE / 8>::operator[]( pos >> 3 ), (uint8_t)( 1 << ( pos & 7 ) ) };
    }
  };

  void helper( std::span<std::shared_ptr<IMemoryAccessTrap>> dest, uint16_t address, std::shared_ptr<IMemoryAccessTrap> src )
  {
    if ( dest[address] )
    {
      auto tmp = std::move( dest[address] );
      dest[address] = std::make_shared<CompositeTrap>( std::move( tmp ), std::move( src ) );
    }
    else
    {
      dest[address] = std::move( src );
    }
  }

  void helper( std::span<std::shared_ptr<IMemoryAccessTrap>> dest, Proxy proxy, uint16_t address, std::shared_ptr<IMemoryAccessTrap> src )
  {
    if ( proxy )
    {
      auto tmp = std::move( dest[address] );
      dest[address] = std::make_shared<CompositeTrap>( std::move( tmp ), std::move( src ) );
    }
    else
    {
      dest[address] = std::move( src );
      proxy = true;
    }
  }


private:
  BitArray<65536> mRamReadMask;
  std::array<std::shared_ptr<IMemoryAccessTrap>, 65536> mRamReadTraps;
  BitArray<65536> mRamWriteMask;
  std::array<std::shared_ptr<IMemoryAccessTrap>, 65536> mRamWriteTraps;
  BitArray<65536> mRamExecuteMask;
  std::array<std::shared_ptr<IMemoryAccessTrap>, 65536> mRamExecuteTraps;

  BitArray<512> mRomReadMask;
  std::array<std::shared_ptr<IMemoryAccessTrap>, 512> mRomReadTraps;
  BitArray<512> mRomWriteMask;
  std::array<std::shared_ptr<IMemoryAccessTrap>, 512> mRomWriteTraps;
  BitArray<512> mRomExecuteMask;
  std::array<std::shared_ptr<IMemoryAccessTrap>, 512> mRomExecuteTraps;

  BitArray<256> mMikeyReadMask;
  std::array<std::shared_ptr<IMemoryAccessTrap>, 256> mMikeyReadTraps;
  BitArray<256> mMikeyWriteMask;
  std::array<std::shared_ptr<IMemoryAccessTrap>, 256> mMikeyWriteTraps;

  BitArray<256> mSuzyReadMask;
  std::array<std::shared_ptr<IMemoryAccessTrap>, 256> mSuzyReadTraps;
  BitArray<256> mSuzyWriteMask;
  std::array<std::shared_ptr<IMemoryAccessTrap>, 256> mSuzyWriteTraps;

  std::shared_ptr<IMemoryAccessTrap> mMapCtlReadTrap;
  std::shared_ptr<IMemoryAccessTrap> mMapCtlWriteTrap;
};


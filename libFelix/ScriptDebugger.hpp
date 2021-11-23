#pragma once

#include "IMemoryAccessTrap.hpp"
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
  };

public:

  enum class Type : uint16_t
  {
    RAM_READ,
    RAM_WRITE,
    RAM_EXECUTE,
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

  void addTrap( Type type, uint16_t address, std::shared_ptr<IMemoryAccessTrap> trap )
  {
    switch ( type )
    {
    case Type::RAM_READ:
      helper( mRamReadTraps[address], std::move( trap ) );
      break;
    case Type::RAM_WRITE:
      helper( mRamWriteTraps[address], std::move( trap ) );
      break;
    case Type::RAM_EXECUTE:
      helper( mRamExecuteTraps[address], std::move( trap ) );
      break;
    case Type::ROM_READ:
      helper( mRomReadTraps[address & 0x1ff], std::move( trap ) );
      break;
    case Type::ROM_WRITE:
      helper( mRomWriteTraps[address & 0x1ff], std::move( trap ) );
      break;
    case Type::ROM_EXECUTE:
      helper( mRomExecuteTraps[address & 0x1ff], std::move( trap ) );
      break;
    case Type::MIKEY_READ:
      helper( mMikeyReadTraps[address & 0xff], std::move( trap ) );
      break;
    case Type::MIKEY_WRITE:
      helper( mMikeyWriteTraps[address & 0xff], std::move( trap ) );
      break;
    case Type::SUZY_READ:
      helper( mSuzyReadTraps[address & 0xff], std::move( trap ) );
      break;
    case Type::SUZY_WRITE:
      helper( mSuzyWriteTraps[address & 0xff], std::move( trap ) );
      break;
    case Type::MAPCTL_READ:
      helper( mMapCtlReadTrap, std::move( trap ) );
      break;
    case Type::MAPCTL_WRITE:
      helper( mMapCtlWriteTrap, std::move( trap ) );
      break;
    }
  }

  uint8_t readRAM( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( auto trap = mRamReadTraps[address].get() )
    {
      return trap->trap( core, address, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t writeRAM( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( auto trap = mRamWriteTraps[address].get() )
    {
      return trap->trap( core, address, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t executeRAM( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( auto trap = mRamExecuteTraps[address].get() )
    {
      return trap->trap( core, address, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t readROM( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( auto trap = mRomReadTraps[address].get() )
    {
      return trap->trap( core, address + 0xfe00, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t writeROM( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( auto trap = mRomWriteTraps[address].get() )
    {
      return trap->trap( core, address + 0xfe00, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t executeROM( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( auto trap = mRomExecuteTraps[address].get() )
    {
      return trap->trap( core, address + 0xfe00, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t readMikey( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( auto trap = mMikeyReadTraps[address & 0xff].get() )
    {
      return trap->trap( core, address, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t writeMikey( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( auto trap = mMikeyWriteTraps[address & 0xff].get() )
    {
      return trap->trap( core, address, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t readSuzy( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( auto trap = mSuzyReadTraps[address & 0xff].get() )
    {
      return trap->trap( core, address, orgValue );
    }
    else
    {
      return orgValue;
    }
  }

  uint8_t writeSuzy( Core& core, uint16_t address, uint8_t orgValue )
  {
    if ( auto trap = mSuzyWriteTraps[address & 0xff].get() )
    {
      return trap->trap( core, address, orgValue );
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
  void helper( std::shared_ptr<IMemoryAccessTrap>& dest, std::shared_ptr<IMemoryAccessTrap> src )
  {
    if ( dest )
    {
      auto tmp = std::move( dest );
      dest = std::make_shared<CompositeTrap>( std::move( tmp ), std::move( src ) );
    }
    else
    {
      dest = std::move( src );
    }
  }

private:
  //TODO: compare performance with unordered map
  std::array<std::shared_ptr<IMemoryAccessTrap>, 65536> mRamReadTraps;
  std::array<std::shared_ptr<IMemoryAccessTrap>, 65536> mRamWriteTraps;
  std::array<std::shared_ptr<IMemoryAccessTrap>, 65536> mRamExecuteTraps;

  std::array<std::shared_ptr<IMemoryAccessTrap>, 512> mRomReadTraps;
  std::array<std::shared_ptr<IMemoryAccessTrap>, 512> mRomWriteTraps;
  std::array<std::shared_ptr<IMemoryAccessTrap>, 512> mRomExecuteTraps;

  std::array<std::shared_ptr<IMemoryAccessTrap>, 256> mMikeyReadTraps;
  std::array<std::shared_ptr<IMemoryAccessTrap>, 256> mMikeyWriteTraps;

  std::array<std::shared_ptr<IMemoryAccessTrap>, 256> mSuzyReadTraps;
  std::array<std::shared_ptr<IMemoryAccessTrap>, 256> mSuzyWriteTraps;

  std::shared_ptr<IMemoryAccessTrap> mMapCtlReadTrap;
  std::shared_ptr<IMemoryAccessTrap> mMapCtlWriteTrap;
};


#pragma once

#include "IMemoryAccessTrap.hpp"
class Core;

class ScriptDebugger
{
public:
  ScriptDebugger();
  ~ScriptDebugger();

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


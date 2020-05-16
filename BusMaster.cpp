#include "BusMaster.hpp"
#include "CPU.hpp"
#include "Suzy.hpp"
#include "Mikey.hpp"
#include <fstream>
#include <filesystem>
#include <cassert>

BusMaster::BusMaster() : mRAM{}, mROM{}, mPageTypes{}, mBusReservationTick{}, mCurrentTick{},
  mCpu{ std::make_shared<CPU>() }, mMikey{ std::make_shared<Mikey>( *this ) }, mSuzy{ std::make_shared<Suzy>() },
  mDReq{}, mCPUReq{}, mSuzyReq{}, mCpuExecute{ mCpu->execute() }, mSuzyExecute{}, mCpuTrace{ /*cpuTrace( *mCpu, mDReq )*/ },
  mActionQueue{}, mMapCtl{}, mSequencedAccessAddress{ ~0u }, mDMAAddress{}, mFastCycleTick{ 4 }
{
  {
    std::ifstream fin{ "d:/test/tests/lynxboot.img", std::ios::binary };
    if ( fin.bad() )
      throw std::exception{};

    fin.read( ( char* )mROM.data(), mROM.size() );
  }
  if ( size_t size = (size_t)std::filesystem::file_size( "d:/test/tests/7.o" ) )
  {
    std::ifstream fin{ "d:/test/tests/7.o", std::ios::binary };
    if ( fin.bad() )
      throw std::exception{};

    fin.read( ( char* )mRAM.data() + 0x1f6, size );

    mROM[0x1fc] = 0x00;
    mROM[0x1fd] = 0x02;
  }

  for ( size_t i = 0; i < mPageTypes.size(); ++i )
  {
    switch ( i )
    {
      case 0xff:
        mPageTypes[i] = PageType::FF;
        break;
      case 0xfe:
        mPageTypes[i] = PageType::FE;
        break;
      case 0xfd:
        mPageTypes[i] = PageType::MIKEY;
        break;
      case 0xfc:
        mPageTypes[i] = PageType::SUZY;
        break;
      default:
        mPageTypes[i] = PageType::RAM;
        break;
    }
  }

  mCpuTrace = cpuTrace( *mCpu, mDReq );
  mCpuExecute.setBusMaster( this );
}

BusMaster::~BusMaster()
{
}

CPURequest * BusMaster::request( CPURead r )
{
  mCPUReq = CPURequest{ r };
  processCPU();
  return &mCPUReq;
}

CPURequest * BusMaster::request( CPUFetchOpcode r )
{
  mCPUReq = CPURequest{ r };
  processCPU();
  return &mCPUReq;
}

CPURequest * BusMaster::request( CPUFetchOperand r )
{
  mCPUReq = CPURequest{ r };
  processCPU();
  return &mCPUReq;
}

CPURequest * BusMaster::request( CPUWrite w )
{
  mCPUReq = CPURequest{ w };
  processCPU();
  return &mCPUReq;
}

SuzyRequest * BusMaster::suzyRequest()
{
  return &mSuzyReq;
}

void BusMaster::requestDisplayDMA( uint64_t tick, uint16_t address )
{
  mDMAAddress = address;
  mActionQueue.push( { Action::DISPLAY_DMA, tick } );
}

DisplayGenerator::Pixel const* BusMaster::process( uint64_t ticks )
{
  mActionQueue.push( { Action::END_FRAME, mCurrentTick + ticks } );

  for ( ;; )
  {
    auto seqAction = mActionQueue.pop();
    mCurrentTick = seqAction.getTick();
    auto action = seqAction.getAction();

    switch ( action )
    {
      case Action::DISPLAY_DMA:
        mMikey->setDMAData( mCurrentTick, *(uint64_t*)( mRAM.data() + mDMAAddress ) );
        mBusReservationTick += 6 * mFastCycleTick + 2 * 5;
        break;
      case Action::FIRE_TIMER0:
      case Action::FIRE_TIMER1:
      case Action::FIRE_TIMER2:
      case Action::FIRE_TIMER3:
      case Action::FIRE_TIMER4:
      case Action::FIRE_TIMER5:
      case Action::FIRE_TIMER6:
      case Action::FIRE_TIMER7:
      case Action::FIRE_TIMER8:
      case Action::FIRE_TIMER9:
      case Action::FIRE_TIMERA:
      case Action::FIRE_TIMERB:
      case Action::FIRE_TIMERC:
      if ( auto newAction = mMikey->fireTimer( mCurrentTick, ( int )action - ( int )Action::FIRE_TIMER0 ) )
      {
        mActionQueue.push( newAction );
      }
      break;
    case Action::CPU_FETCH_OPCODE_RAM:
      mCPUReq.value = mRAM[mCPUReq.address];
      mSequencedAccessAddress = mCPUReq.address + 1;
      mCurrentTick;
      mCPUReq.tick = mCurrentTick;
      mCPUReq.interrupt = mMikey->getIRQ() != 0 ? CPU::I_IRQ : 0;
      mCPUReq();
      mDReq.resume();
      break;
    case Action::CPU_FETCH_OPERAND_RAM:
      mCPUReq.value = mRAM[mCPUReq.address];
      mSequencedAccessAddress = mCPUReq.address + 1;
      mCPUReq();
      mDReq.resume();
      break;
    case Action::CPU_READ_RAM:
      mCPUReq.value = mRAM[mCPUReq.address];
      mSequencedAccessAddress = mCPUReq.address + 1;
      mCPUReq();
      break;
    case Action::CPU_WRITE_RAM:
      mRAM[mCPUReq.address] = mCPUReq.value;
      mSequencedAccessAddress = ~0;
      mCPUReq();
      break;
    case Action::CPU_FETCH_OPCODE_FE:
      mCPUReq.value = mROM[mCPUReq.address & 0x1ff];
      mSequencedAccessAddress = mCPUReq.address + 1;
      mCurrentTick;
      mCPUReq.tick = mCurrentTick;
      mCPUReq.interrupt = mMikey->getIRQ() != 0 ? CPU::I_IRQ : 0;
      mCPUReq();
      mDReq.resume();
      break;
    case Action::CPU_FETCH_OPERAND_FE:
      mCPUReq.value = mROM[mCPUReq.address & 0x1ff];
      mSequencedAccessAddress = mCPUReq.address + 1;
      mCPUReq();
      mDReq.resume();
      break;
    case Action::CPU_READ_FE:
      mCPUReq.value = mROM[mCPUReq.address & 0x1ff];
      mSequencedAccessAddress = mCPUReq.address + 1;
      mCPUReq();
      break;
    case Action::CPU_WRITE_FE:
      mSequencedAccessAddress = ~0;
      mCPUReq();
      break;
    case Action::CPU_FETCH_OPCODE_FF:
      mCPUReq.value = readFF( mCPUReq.address & 0xff );
      mSequencedAccessAddress = mCPUReq.address + 1;
      mCurrentTick;
      mCPUReq.tick = mCurrentTick;
      mCPUReq.interrupt = mMikey->getIRQ() != 0 ? CPU::I_IRQ : 0;
      mCPUReq();
      mDReq.resume();
      break;
    case Action::CPU_FETCH_OPERAND_FF:
      mCPUReq.value = readFF( mCPUReq.address & 0xff );
      mSequencedAccessAddress = mCPUReq.address + 1;
      mCPUReq();
      mDReq.resume();
      break;
    case Action::CPU_READ_FF:
      mCPUReq.value = readFF( mCPUReq.address & 0xff );
      mSequencedAccessAddress = mCPUReq.address + 1;
      mCPUReq();
      break;
    case Action::CPU_WRITE_FF:
      writeFF( mCPUReq.address & 0xff, mCPUReq.value );
      mSequencedAccessAddress = ~0;
      mCPUReq();
      break;
    case Action::CPU_READ_SUZY:
      mCPUReq.value = mSuzy->read( mCPUReq.address );
      mCPUReq();
      break;
    case Action::CPU_WRITE_SUZY:
      mSuzy->write( mCPUReq.address, mCPUReq.value );
      mCPUReq();
      break;
    case Action::CPU_READ_MIKEY:
      mCPUReq.value = mMikey->read( mCPUReq.address );
      mCPUReq();
      break;
    case Action::CPU_WRITE_MIKEY:
      switch ( auto mikeyAction = mMikey->write( mCPUReq.address, mCPUReq.value ) )
      {
      case Mikey::WriteAction::Type::START_SUZY:
        mSuzyExecute = mSuzy->processSprites( *this );
        processSuzy();
        break;
      case Mikey::WriteAction::Type::ENQUEUE_ACTION:
        mActionQueue.push( mikeyAction.action );
        [[fallthrough]];
      case Mikey::WriteAction::Type::NONE:
        mCPUReq();
        break;
      }
      break;
    case Action::SUZY_NONE:
      mCPUReq();
      break;
    case Action::SUZY_READ:
      mSuzyReq.value = mRAM[mSuzyReq.address];
      processSuzy();
      break;
    case Action::SUZY_READ4:
      assert( mSuzyReq.address <= 0xfffc );
      mSuzyReq.value = *( (uint32_t const*)( mRAM.data() + mSuzyReq.address ) );
      processSuzy();
      break;
    case Action::SUZY_WRITE:
      mRAM[mSuzyReq.address] = (uint8_t)mSuzyReq.value;
      processSuzy();
      break;
    case Action::SUZY_WRITE4:
      assert( mSuzyReq.address <= 0xfffc );
      *( ( uint32_t* )( mRAM.data() + mSuzyReq.address ) ) =  mSuzyReq.value;
      processSuzy();
      break;
    case Action::SUZY_MASKED_RMW:
      suzyMaskedRMW();
      processSuzy();
      break;
    case Action::SUZY_XOR:
      suzyXor();
      processSuzy();
      break;
    case Action::END_FRAME:
      return mMikey->getSrface();
    case Action::CPU_FETCH_OPCODE_SUZY:
    case Action::CPU_FETCH_OPCODE_MIKEY:
    case Action::CPU_FETCH_OPERAND_SUZY:
    case Action::CPU_FETCH_OPERAND_MIKEY:
      assert( false );
      break;
    default:
      break;
    }
  }
}

void BusMaster::enterMonitor()
{
}

void BusMaster::suzyMaskedRMW()
{
  auto value = mRAM[mSuzyReq.address] & mSuzyReq.mask | mSuzyReq.value;
  mRAM[mSuzyReq.address] = ( uint8_t )value;
}

void BusMaster::suzyXor()
{
  auto value = mRAM[mSuzyReq.address] ^ mSuzyReq.value;
  mRAM[mSuzyReq.address] = ( uint8_t )value;
}

void BusMaster::processSuzy()
{
  struct Cost
  {
    int slow;
    int fast;
  };

  static constexpr std::array<Cost, ( int )::SuzyRequest::Op::_SIZE> requestCost = {
    Cost{ 0, 0 }, //NONE,
    Cost{ 1, 0 }, //READ,
    Cost{ 1, 3 }, //READ4,
    Cost{ 1, 0 }, //WRITE,
    Cost{ 1, 3 }, //WRITE4,
    Cost{ 1, 1 }, //MASKED_RMW,
    Cost{ 1, 1 }  //XOR,
  };

  mSuzyReq();
  int op = ( int )mSuzyReq.operation;
  mActionQueue.push( { ( Action )( ( int )Action::SUZY_NONE + op ), mBusReservationTick } );
  auto cost = requestCost[op];
  mBusReservationTick += cost.slow * 5ull + cost.fast * mFastCycleTick;
}

void BusMaster::processCPU()
{
  static constexpr std::array<Action, 25> requestToAction = {
    Action::NONE,
    Action::CPU_FETCH_OPCODE_RAM,
    Action::CPU_FETCH_OPERAND_RAM,
    Action::CPU_READ_RAM,
    Action::CPU_WRITE_RAM,
    Action::NONE_FE,
    Action::CPU_FETCH_OPCODE_FE,
    Action::CPU_FETCH_OPERAND_FE,
    Action::CPU_READ_FE,
    Action::CPU_WRITE_FE,
    Action::NONE_FF,
    Action::CPU_FETCH_OPCODE_FF,
    Action::CPU_FETCH_OPERAND_FF,
    Action::CPU_READ_FF,
    Action::CPU_WRITE_FF,
    Action::NONE_MIKEY,
    Action::CPU_FETCH_OPCODE_MIKEY,
    Action::CPU_FETCH_OPERAND_MIKEY,
    Action::CPU_READ_MIKEY,
    Action::CPU_WRITE_MIKEY,
    Action::NONE_SUZY,
    Action::CPU_FETCH_OPCODE_SUZY,
    Action::CPU_FETCH_OPERAND_SUZY,
    Action::CPU_READ_SUZY,
    Action::CPU_WRITE_SUZY
  };

  auto pageType = mPageTypes[mCPUReq.address >> 8];
  mActionQueue.push( { requestToAction[( size_t )mCPUReq.mType + ( int )pageType], mBusReservationTick } );
  switch ( pageType )
  {
    case PageType::RAM:
    case PageType::FE:
    case PageType::FF:
      mBusReservationTick += ( mCPUReq.address == mSequencedAccessAddress ) ? mFastCycleTick : 5;
      break;
    case PageType::MIKEY:
      mBusReservationTick = mMikey->requestAccess( mBusReservationTick, mCPUReq.address );
      break;
    case PageType::SUZY:
      mBusReservationTick = mSuzy->requestAccess( mBusReservationTick, mCPUReq.address );
      break;
  }
}

uint8_t BusMaster::readFF( uint16_t address )
{
  if ( address >= 0xfa )
  {
    uint8_t * ptr = mMapCtl.vectorSpaceDisable ? ( mRAM.data() + 0xff00 ) : ( mROM.data() + 0x100 );
    return ptr[address];
  }
  else if ( address < 0xf8 )
  {
    uint8_t * ptr = mMapCtl.kernelDisable ? ( mRAM.data() + 0xff00 ) : ( mROM.data() + 0x100 );
    return ptr[address];
  }
  else if ( address == 0xf9 )
  {
    return 0xf0 | //high nibble of MAPCTL is set
      ( mMapCtl.vectorSpaceDisable ? 0x08 : 0x00 ) |
      ( mMapCtl.kernelDisable ? 0x04 : 0x00 ) |
      ( mMapCtl.mikeyDisable ? 0x02 : 0x00 ) |
      ( mMapCtl.suzyDisable ? 0x01 : 0x00 );
  }
  else
  {
    //there is always RAM at 0xfff8
    return mRAM[0xff00 + address];
  }
}

void BusMaster::writeFF( uint16_t address, uint8_t value )
{
  if ( address >= 0xfa && mMapCtl.vectorSpaceDisable || address < 0xf8 && mMapCtl.kernelDisable || address == 0xf8 )
  {
    mRAM[0xff00 + address] = value;
  }
  else if ( address == 0xf9 )
  {
    mMapCtl.sequentialDisable = ( value & 0x80 ) != 0;
    mMapCtl.vectorSpaceDisable = ( value & 0x08 ) != 0;
    mMapCtl.kernelDisable = ( value & 0x04 ) != 0;
    mMapCtl.mikeyDisable = ( value & 0x02 ) != 0;
    mMapCtl.suzyDisable = ( value & 0x01 ) != 0;

    mFastCycleTick = mMapCtl.sequentialDisable ? 5 : 4;
    mPageTypes[0xfe] = mMapCtl.kernelDisable ? PageType::RAM : PageType::FE;
    mPageTypes[0xfd] = mMapCtl.mikeyDisable ? PageType::RAM : PageType::MIKEY;
    mPageTypes[0xfc] = mMapCtl.suzyDisable ? PageType::RAM : PageType::SUZY;
  }
  else
  {
    //ignore write to ROM
  }
}

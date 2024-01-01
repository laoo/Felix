#pragma once

#include "ParallelPort.hpp"
#include "Utility.hpp"

struct IVideoSink;

class DisplayGenerator : public RestProvider
{
public:


public:

  struct DMARequest
  {
    uint64_t tick;
    uint16_t address;

    explicit operator bool() const
    {
      return tick != 0;
    }
  };

  DisplayGenerator( std::shared_ptr<IVideoSink> videoSink );
  ~DisplayGenerator() override = default;
  void dispCtl( bool dispColor, bool dispFlip, bool dmaEnable );
  void setPBKUP( uint8_t value );

  void vblank( uint64_t tick );
  DMARequest hblank( uint64_t tick, int row );
  DMARequest pushData( uint64_t tick, uint64_t data );
  void updateDispAddr( uint64_t tick, uint16_t dispAdr );

  void setColorReg( uint64_t tick, uint8_t reg, uint8_t value );
  uint8_t getColorReg( uint8_t reg ) const;
  std::span<uint8_t const, 32> debugPalette() const;

  bool rest() const override;

private:
  void flushDisplay( uint64_t tick );

private:

  std::array<Doublet, 256> mDoublets;
  std::array<uint8_t, 32> mPalette;
  std::array<uint64_t,10> mDMAData;
  std::shared_ptr<IVideoSink> mVideoSink;
  uint64_t mRowStartTick;
  uint32_t mDMAIteration;
  int32_t mDisplayRow;
  uint32_t mEmittedRowDoublets;
  uint16_t mDispAdr;
  bool mDispColor;
  bool mDispFlip;
  bool mDMAEnable;
  int mDMAOffset;
  Doublet* mRowPtr;

  static constexpr uint64_t DMA_FETCH_SIZE = 8;
  static constexpr uint64_t TICKS_PER_PIXEL = 12;
  static constexpr uint64_t DMA_ITERATIONS = ROW_BYTES / DMA_FETCH_SIZE;
  static constexpr uint64_t TICKS_PER_BYTE = TICKS_PER_PIXEL * 2;
  static constexpr uint64_t ROW_TICKS = TICKS_PER_BYTE * ROW_BYTES;


};

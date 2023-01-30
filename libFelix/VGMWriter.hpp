#pragma once

class VGMWriter
{
public:
  VGMWriter( std::filesystem::path path );
  ~VGMWriter();

  void init( uint64_t tick );
  void write( uint64_t tick, uint8_t reg, uint8_t val );

private:
  uint32_t tickToSample( uint64_t tick ) const;

private:

  static constexpr uint64_t MIKEY_CLOCK = 16000000;
  static constexpr uint64_t SAMPLE_RATE = 44100;
  static constexpr uint64_t MIKEY_CLOCK_OFFSET = 0xe4;
  static constexpr char CMD_MIKEY = 0x40;
  static constexpr char CMD_LONG_WAIT = 0x61;
  static constexpr char CMD_SHORT_WAIT = 0x70;
  static constexpr char CMD_END_OF_SOUND_DATA = 0x66;

  static constexpr uint32_t CMD_LONG_WAIT_MAX = 0xffff;
  static constexpr uint32_t CMD_SHORT_WAIT_MAX = 0x10;

  struct VGMHeader
  {
    std::array<char const, 4> ident = { 0x56, 0x67, 0x6d, 0x20 };
    uint32_t EofOffset = 0;
    uint32_t VersionNumber = 0x172;
    uint32_t SN76489_clock = 0;
    uint32_t YM2413_clock = 0;
    uint32_t GD3_offset = 0;
    uint32_t Total_samples = 0;
    uint32_t Loop_offset = 0;
    uint32_t Loop_samples = 0;
    uint32_t Rate = 0;
    uint16_t SN76489_feedback = 0;
    uint8_t SN76489_shift_register_width = 0;
    uint8_t SN76489_Flags = 0;
    uint32_t YM2612_clock = 0;
    uint32_t YM2151_clock = 0;
    uint32_t VGM_data_offset = sizeof( VGMHeader ) - offsetof( VGMHeader, VGM_data_offset );
    uint32_t Sega_PCM_clock = 0;
    uint32_t Sega_PCM_interface_register = 0;
    uint32_t RF5C68_clock = 0;
    uint32_t YM2203_clock = 0;
    uint32_t YM2608_clock = 0;
    uint32_t YM2610_YM2610B_clock = 0;
    uint32_t YM3812_clock = 0;
    uint32_t YM3526_clock = 0;
    uint32_t Y8950_clock = 0;
    uint32_t YMF262_clock = 0;
    uint32_t YMF278B_clock = 0;
    uint32_t YMF271_clock = 0;
    uint32_t YMZ280B_clock = 0;
    uint32_t RF5C164_clock = 0;
    uint32_t PWM_clock = 0;
    uint32_t AY8910_clock = 0;
    uint8_t AY8910_Chip_Type = 0;
    uint8_t AY8910_Flags = 0;
    uint8_t YM2203_AY8910_Flags = 0;
    uint8_t YM2608_AY8910_Flags = 0;
    uint8_t Volume_Modifier = 0;
    uint8_t reserved_7d = 0;
    uint8_t Loop_Base = 0;
    uint8_t Loop_Modifier = 0;
    uint32_t GameBoy_DMG_clock = 0;
    uint32_t NES_APU_clock = 0;
    uint32_t MultiPCM_clock = 0;
    uint32_t uPD7759_clock = 0;
    uint32_t OKIM6258_clock = 0;
    uint8_t OKIM6258_Flags = 0;
    uint8_t K054539_Flags = 0;
    uint8_t C140_Chip_Type = 0;
    uint8_t reserved_97 = 0;
    uint32_t OKIM6295_clock = 0;
    uint32_t K051649_clock = 0;
    uint32_t K054539_clock = 0;
    uint32_t HuC6280_clock = 0;
    uint32_t C140_clock = 0;
    uint32_t K053260_clock = 0;
    uint32_t Pokey_clock = 0;
    uint32_t QSound_clock = 0;
    uint32_t SCSP_clock = 0;
    uint32_t Extra_Header_Offset = 0;
    uint32_t WonderSwan_clock = 0;
    uint32_t VSU_clock = 0;
    uint32_t SAA1099_clock = 0;
    uint32_t ES5503_clock = 0;
    uint32_t ES5505_ES5506_clock = 0;
    uint8_t ES5503_amount_of_output_channels = 0;
    uint8_t ES5505_ES5506_amount_of_output_channels = 0;
    uint8_t C352_clock_divider = 0;
    uint8_t reserved_d7 = 0;
    uint32_t X1_010_clock = 0;
    uint32_t C352_clock = 0;
    uint32_t GA20_clock = 0;
    uint32_t Mikey_clock = MIKEY_CLOCK;
  } mHeader;

  static_assert( sizeof( VGMHeader ) == 0xe8 );

  std::ofstream mFout;
  uint64_t mLastTick;
};

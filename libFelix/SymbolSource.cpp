#include "pch.hpp"
#include "SymbolSource.hpp"

namespace
{
static constexpr std::pair<char const*, uint16_t> defaultSymbols[] = {
  { "ATTENREG0", 0xfd40 },
  { "ATTENREG1", 0xfd41 },
  { "ATTENREG2", 0xfd42 },
  { "ATTENREG3", 0xfd43 },
  { "AUDIN", 0xfd86 },
  { "AUDIO0_BACKUP", 0xfd24 },
  { "AUDIO0_CONTROL", 0xfd25 },
  { "AUDIO0_COUNTER", 0xfd26 },
  { "AUDIO0_FEEDBACK", 0xfd21 },
  { "AUDIO0_OTHER", 0xfd27 },
  { "AUDIO0_OUTPUT", 0xfd22 },
  { "AUDIO0_SHIFT", 0xfd23 },
  { "AUDIO0_VOLCNTRL", 0xfd20 },
  { "AUDIO1_BACKUP", 0xfd2c },
  { "AUDIO1_CONTROL", 0xfd2d },
  { "AUDIO1_COUNTER", 0xfd2e },
  { "AUDIO1_FEEDBACK", 0xfd29 },
  { "AUDIO1_OTHER", 0xfd2f },
  { "AUDIO1_OUTPUT", 0xfd2a },
  { "AUDIO1_SHIFT", 0xfd2b },
  { "AUDIO1_VOLCNTRL", 0xfd28 },
  { "AUDIO2_BACKUP", 0xfd34 },
  { "AUDIO2_BACKUP", 0xfd3c },
  { "AUDIO2_CONTROL", 0xfd35 },
  { "AUDIO2_CONTROL", 0xfd3d },
  { "AUDIO2_COUNTER", 0xfd36 },
  { "AUDIO2_COUNTER", 0xfd3e },
  { "AUDIO2_FEEDBACK", 0xfd31 },
  { "AUDIO2_FEEDBACK", 0xfd39 },
  { "AUDIO2_OTHER", 0xfd37 },
  { "AUDIO2_OTHER", 0xfd3f },
  { "AUDIO2_OUTPUT", 0xfd32 },
  { "AUDIO2_OUTPUT", 0xfd3a },
  { "AUDIO2_SHIFT", 0xfd33 },
  { "AUDIO2_SHIFT", 0xfd3b },
  { "AUDIO2_VOLCNTRL", 0xfd30 },
  { "AUDIO2_VOLCNTRL", 0xfd38 },
  { "BLUERED0", 0xfdb0 },
  { "BLUERED1", 0xfdb1 },
  { "BLUERED2", 0xfdb2 },
  { "BLUERED3", 0xfdb3 },
  { "BLUERED4", 0xfdb4 },
  { "BLUERED5", 0xfdb5 },
  { "BLUERED6", 0xfdb6 },
  { "BLUERED7", 0xfdb7 },
  { "BLUERED8", 0xfdb8 },
  { "BLUERED9", 0xfdb9 },
  { "BLUEREDA", 0xfdba },
  { "BLUEREDB", 0xfdbb },
  { "BLUEREDC", 0xfdbc },
  { "BLUEREDD", 0xfdbd },
  { "BLUEREDE", 0xfdbe },
  { "BLUEREDF", 0xfdbf },
  { "COLLADR", 0xfc0e },
  { "COLLBAS", 0xfc0a },
  { "COLLOFF", 0xfc24 },
  { "CPU_IRQ", 0xfffe },
  { "CPU_NMI", 0xfffa },
  { "CPU_RESET", 0xfffc },
  { "CPUSLEEP", 0xfd91 },
  { "DISPADR", 0xfd94 },
  { "DISPCTL", 0xfd92 },
  { "GREEN0", 0xfda0 },
  { "GREEN1", 0xfda1 },
  { "GREEN2", 0xfda2 },
  { "GREEN3", 0xfda3 },
  { "GREEN4", 0xfda4 },
  { "GREEN5", 0xfda5 },
  { "GREEN6", 0xfda6 },
  { "GREEN7", 0xfda7 },
  { "GREEN8", 0xfda8 },
  { "GREEN9", 0xfda9 },
  { "GREENA", 0xfdaa },
  { "GREENB", 0xfdab },
  { "GREENC", 0xfdac },
  { "GREEND", 0xfdad },
  { "GREENE", 0xfdae },
  { "GREENF", 0xfdaf },
  { "HCOUNT_BACKUP", 0xfd00 },
  { "HCOUNT_CONTROLA", 0xfd01 },
  { "HCOUNT_CONTROLB", 0xfd03 },
  { "HCOUNT_COUNT", 0xfd02 },
  { "HOFF", 0xfc04 },
  { "HOWIE", 0xfcc4 },
  { "HPOSSTRT", 0xfc14 },
  { "HSIZOFF", 0xfc28 },
  { "INTRST", 0xfd80 },
  { "INTSET", 0xfd81 },
  { "IODAT", 0xfd8b },
  { "IODATA", 0xfcc3 },
  { "IODIR", 0xfd8a },
  { "IOSTATUS", 0xfcc2 },
  { "JOYSTICK", 0xfcb0 },
  { "LEDS", 0xfcc0 },
  { "MAPCTL", 0xfff9 },
  { "MATHA", 0xfc55 },
  { "MATHB", 0xfc54 },
  { "MATHC", 0xfc53 },
  { "MATHD", 0xfc52 },
  { "MATHE", 0xfc63 },
  { "MATHF", 0xfc62 },
  { "MATHG", 0xfc61 },
  { "MATHH", 0xfc60 },
  { "MATHJ", 0xfc6f },
  { "MATHK", 0xfc6e },
  { "MATHL", 0xfc6d },
  { "MATHM", 0xfc6c },
  { "MATHN", 0xfc57 },
  { "MATHP", 0xfc56 },
  { "MIKEYHREV", 0xfd88 },
  { "MIKEYSREV", 0xfd89 },
  { "MPAN", 0xfd44 },
  { "MTEST0", 0xfd9c },
  { "MTEST1", 0xfd9d },
  { "MTEST2", 0xfd9e },
  { "PBKUP", 0xfd93 },
  { "PROCADR", 0xfc2e },
  { "RCART0", 0xfcb2 },
  { "RCART1", 0xfcb3 },
  { "SCBADR", 0xfc2c },
  { "SCBNEXT", 0xfc10 },
  { "SCVPOS", 0xfc22 },
  { "SDONEACK", 0xfd90 },
  { "SERCTL", 0xfd8c },
  { "SERDAT", 0xfd8d },
  { "SERIALRATE_BACKUP", 0xfd10 },
  { "SERIALRATE_CONTROLA", 0xfd11 },
  { "SERIALRATE_CONTROLB", 0xfd13 },
  { "SERIALRATE_COUNT", 0xfd12 },
  { "SPRCOLL", 0xfc82 },
  { "SPRCTL0", 0xfc80 },
  { "SPRCTL1", 0xfc81 },
  { "SPRDLINE", 0xfc12 },
  { "SPRDOFF", 0xfc20 },
  { "SPRGO", 0xfc91 },
  { "SPRHSIZ", 0xfc18 },
  { "SPRINIT", 0xfc83 },
  { "SPRSYS", 0xfc92 },
  { "SPRVSIZ", 0xfc1a },
  { "STEREO", 0xfd50 },
  { "STRETCH", 0xfc1c },
  { "SUZYBUSEN", 0xfc90 },
  { "SUZYHREV", 0xfc88 },
  { "SUZYSREV", 0xfc89 },
  { "SWITCHES", 0xfcb1 },
  { "SYSCTL1", 0xfd87 },
  { "TILT", 0xfc1e },
  { "TILTACUM", 0xfc02 },
  { "TIMER1_BACKUP", 0xfd04 },
  { "TIMER1_CONTROLA", 0xfd05 },
  { "TIMER1_CONTROLB", 0xfd07 },
  { "TIMER1_COUNT", 0xfd06 },
  { "TIMER3_BACKUP", 0xfd0c },
  { "TIMER3_CONTROLA", 0xfd0d },
  { "TIMER3_CONTROLB", 0xfd0f },
  { "TIMER3_COUNT", 0xfd0e },
  { "TIMER5_BACKUP", 0xfd14 },
  { "TIMER5_CONTROLA", 0xfd15 },
  { "TIMER5_CONTROLB", 0xfd17 },
  { "TIMER5_COUNT", 0xfd16 },
  { "TIMER6_BACKUP", 0xfd18 },
  { "TIMER6_CONTROLA", 0xfd19 },
  { "TIMER6_CONTROLB", 0xfd1b },
  { "TIMER6_COUNT", 0xfd1a },
  { "TIMER7_BACKUP", 0xfd1c },
  { "TIMER7_CONTROLA", 0xfd1d },
  { "TIMER7_CONTROLB", 0xfd1f },
  { "TIMER7_COUNT", 0xfd1e },
  { "TMPADR", 0xfc00 },
  { "VCOUNT_BACKUP", 0xfd08 },
  { "VCOUNT_CONTROLA", 0xfd09 },
  { "VCOUNT_CONTROLB", 0xfd0b },
  { "VCOUNT_COUNT", 0xfd0a },
  { "VIDADR", 0xfc0c },
  { "VIDBAS", 0xfc08 },
  { "VOFF", 0xfc06 },
  { "VPOSSTRT", 0xfc16 },
  { "VSIZACUM", 0xfc26 },
  { "VSIZOFF", 0xfc2a }
};
}

SymbolSource::SymbolSource() : mSymbols{}
{
}

SymbolSource::SymbolSource( std::filesystem::path const& labPath ) : mSymbols{}
{
  if ( labPath.empty() )
    return;

  std::ifstream fin{ labPath };

  std::string line;
  //skip two lines
  std::getline( fin, line );
  std::getline( fin, line );

  while ( std::getline( fin, line ) && !line.empty() )
  {
    if ( auto symbol = parseLine( line ) )
    {
      mSymbols.push_back( std::move( symbol ) );
    }
  }

  std::ranges::sort( mSymbols, {}, &Symbol::name );
}

SymbolSource::~SymbolSource()
{
}

std::optional<uint16_t> SymbolSource::symbol( std::string const& name ) const
{
  std::string upper;
  std::transform( name.cbegin(), name.cend(), std::back_inserter( upper ), []( std::string::value_type c ) { return std::toupper( c ); } );
  auto it = std::ranges::lower_bound( mSymbols, upper, {}, &Symbol::name );

  if ( it != mSymbols.cend() && it->name == upper )
    return it->value;

  auto it2 = std::ranges::lower_bound( defaultSymbols, upper, {}, []( auto const& p ) { return p.first; } );

  return it2 != std::cend( defaultSymbols ) && it2->first == upper ? it2->second : std::optional<uint16_t>{};
}

SymbolSource::Symbol SymbolSource::parseLine( std::string const& line )
{
  std::istringstream is{ line };
  std::string name;
  int bank, adr;

  is >> std::hex >> bank >> adr >> name;

  if ( bank == 0 )
    return { std::move( name ), (uint16_t)adr };
  else
    return {};
}

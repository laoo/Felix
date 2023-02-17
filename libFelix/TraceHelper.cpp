#include "pch.hpp"
#include "TraceHelper.hpp"

static constexpr int LABEL_SIZE_LIMIT = 20;

TraceHelper::TraceHelper() : mLabels{}, mTraceComment{}, mData{}, mCommentView{}, mCommentCursor{}, mEnabled{}
{
  char buf[256];
  for ( size_t i = 0; i < 65536; ++i )
  {
    mLabels[i] = (uint32_t)mData.size();
    char const* ptr = map( (uint16_t)i, buf );
    size_t size = 0;
    do
    {
      mData.push_back( *ptr );
      size += 1;
    } while ( *ptr++ );
    while ( size-- > LABEL_SIZE_LIMIT )
    {
      mData.pop_back();
    }
    mData.back() = '\0';
  }
}

TraceHelper::~TraceHelper()
{
}

void TraceHelper::updateLabel( uint16_t address, const char* label )
{
  if ( address > 0xffff )
  {
    return;
  }

  auto labelLen = strlen( label );
  if ( labelLen <= 0 || labelLen > LABEL_SIZE_LIMIT )
  {
    return;
  }

  auto currPos = mLabels[address];

  std::vector<char>::iterator iterFirst = mData.begin() + currPos;
  auto iterLast = iterFirst;

  while ( *iterLast != 0 )
  {
    iterLast += 1;
    labelLen--;
  }

  mData.erase( iterFirst, iterLast );
  
  iterFirst = mData.begin() + currPos;
  for ( int i = 0; i < strlen( label ); ++i )
  {
    iterFirst = mData.insert( iterFirst, label[i]) + 1;
  }
  
  do 
  {
    mLabels[++address] += labelLen;
  } while ( address < 0xffff );
}

char const * TraceHelper::addressLabel( uint16_t address ) const
{
  return mData.data() + mLabels[address];
}

void TraceHelper::enable( bool cond )
{
  mEnabled = cond;
}

std::shared_ptr<std::string_view> TraceHelper::getTraceComment()
{
  if ( mCommentCursor )
  {
    mCommentView = std::string_view{ mTraceComment.data(), mCommentCursor };
    return std::shared_ptr<std::string_view>{
      &mCommentView,
        [this]( auto p )
      {
        mCommentCursor = 0;
      }
    };
  }
  else
  {
    return {};
  }
}

char const * TraceHelper::map( uint16_t address, char * dest ) const
{
  switch ( address )
  {
  case 0xfc00:
    return "TMPADR";
  case 0xfc01:
    return "TMPADR+1";
  case 0xfc02:
    return "TILTACUM";
  case 0xfc03:
    return "TILTACUM+1";
  case 0xfc04:
    return "HOFF";
  case 0xfc05:
    return "HOFF+1";
  case 0xfc06:
    return "VOFF";
  case 0xfc07:
    return "VOFF+1";
  case 0xfc08:
    return "VIDBAS";
  case 0xfc09:
    return "VIDBAS+1";
  case 0xfc0a:
    return "COLLBAS";
  case 0xfc0b:
    return "COLLBAS+1";
  case 0xfc0c:
    return "VIDADR";
  case 0xfc0d:
    return "VIDADR+1";
  case 0xfc0e:
    return "COLLADR";
  case 0xfc0f:
    return "COLLADR+1";
  case 0xfc10:
    return "SCBNEXT";
  case 0xfc11:
    return "SCBNEXT+1";
  case 0xfc12:
    return "SPRDLINE";
  case 0xfc13:
    return "SPRDLINE+1";
  case 0xfc14:
    return "HPOSSTRT";
  case 0xfc15:
    return "HPOSSTRT+1";
  case 0xfc16:
    return "VPOSSTRT";
  case 0xfc17:
    return "VPOSSTRT+1";
  case 0xfc18:
    return "SPRHSIZ";
  case 0xfc19:
    return "SPRHSIZ+1";
  case 0xfc1a:
    return "SPRVSIZ";
  case 0xfc1b:
    return "SPRVSIZ+1";
  case 0xfc1c:
    return "STRETCH";
  case 0xfc1d:
    return "STRETCH+1";
  case 0xfc1e:
    return "TILT";
  case 0xfc1F:
    return "TILT+1";
  case 0xfc20:
    return "SPRDOFF";
  case 0xfc21:
    return "SPRDOFF+1";
  case 0xfc22:
    return "SCVPOS";
  case 0xfc23:
    return "SCVPOS+1";
  case 0xfc24:
    return "COLLOFF";
  case 0xfc25:
    return "COLLOFF+1";
  case 0xfc26:
    return "VSIZACUM";
  case 0xfc27:
    return "VSIZACUM+1";
  case 0xfc28:
    return "HSIZOFF";
  case 0xfc29:
    return "HSIZOFF+1";
  case 0xfc2a:
    return "VSIZOFF";
  case 0xfc2b:
    return "VSIZOFF+1";
  case 0xfc2c:
    return "SCBADR";
  case 0xfc2d:
    return "SCBADR+1";
  case 0xfc2e:
    return "PROCADR";
  case 0xfc2F:
    return "PROCADR+1";

  case 0xfc52:
    return "MATHD";
  case 0xfc53:
    return "MATHC";
  case 0xfc54:
    return "MATHB";
  case 0xfc55:
    return "MATHA";
  case 0xfc56:
    return "MATHP";
  case 0xfc57:
    return "MATHN";

  case 0xfc60:
    return "MATHH";
  case 0xfc61:
    return "MATHG";
  case 0xfc62:
    return "MATHF";
  case 0xfc63:
    return "MATHE";

  case 0xfc6c:
    return "MATHM";
  case 0xfc6d:
    return "MATHL";
  case 0xfc6e:
    return "MATHK";
  case 0xfc6f:
    return "MATHJ";

  case 0xfc80:
    return "SPRCTL0";
  case 0xfc81:
    return "SPRCTL1";
  case 0xfc82:
    return "SPRCOLL";
  case 0xfc83:
    return "SPRINIT";
  case 0xfc88:
    return "SUZYHREV";
  case 0xfc89:
    return "SUZYSREV";
  case 0xfc90:
    return "SUZYBUSEN";
  case 0xfc91:
    return "SPRGO";
  case 0xfc92:
    return "SPRSYS";
  case 0xfcb0:
    return "JOYSTICK";
  case 0xfcb1:
    return "SWITCHES";
  case 0xfcb2:
    return "RCART0";
  case 0xfcb3:
    return "RCART1";
  case 0xfcc0:
    return "LEDS";
  case 0xfcc2:
    return "IOSTATUS";
  case 0xfcc3:
    return "IODATA";
  case 0xfcc4:
    return "HOWIE";

  case 0xfd00:
    return "HCOUNT_BACKUP";
  case 0xfd01:
    return "HCOUNT_CONTROLA";
  case 0xfd02:
    return "HCOUNT_COUNT";
  case 0xfd03:
    return "HCOUNT_CONTROLB";
  case 0xfd04:
    return "TIMER1_BACKUP";
  case 0xfd05:
    return "TIMER1_CONTROLA";
  case 0xfd06:
    return "TIMER1_COUNT";
  case 0xfd07:
    return "TIMER1_CONTROLB";
  case 0xfd08:
    return "VCOUNT_BACKUP";
  case 0xfd09:
    return "VCOUNT_CONTROLA";
  case 0xfd0a:
    return "VCOUNT_COUNT";
  case 0xfd0b:
    return "VCOUNT_CONTROLB";
  case 0xfd0c:
    return "TIMER3_BACKUP";
  case 0xfd0d:
    return "TIMER3_CONTROLA";
  case 0xfd0e:
    return "TIMER3_COUNT";
  case 0xfd0f:
    return "TIMER3_CONTROLB";
  case 0xfd10:
    return "SERIALRATE_BACKUP";
  case 0xfd11:
    return "SERIALRATE_CONTROLA";
  case 0xfd12:
    return "SERIALRATE_COUNT";
  case 0xfd13:
    return "SERIALRATE_CONTROLB";
  case 0xfd14:
    return "TIMER5_BACKUP";
  case 0xfd15:
    return "TIMER5_CONTROLA";
  case 0xfd16:
    return "TIMER5_COUNT";
  case 0xfd17:
    return "TIMER5_CONTROLB";
  case 0xfd18:
    return "TIMER6_BACKUP";
  case 0xfd19:
    return "TIMER6_CONTROLA";
  case 0xfd1a:
    return "TIMER6_COUNT";
  case 0xfd1b:
    return "TIMER6_CONTROLB";
  case 0xfd1c:
    return "TIMER7_BACKUP";
  case 0xfd1d:
    return "TIMER7_CONTROLA";
  case 0xfd1e:
    return "TIMER7_COUNT";
  case 0xfd1f:
    return "TIMER7_CONTROLB";
  case 0xfd20:
    return "AUDIO0_VOLCNTRL";
  case 0xfd21:
    return "AUDIO0_FEEDBACK";
  case 0xfd22:
    return "AUDIO0_OUTPUT";
  case 0xfd23:
    return "AUDIO0_SHIFT";
  case 0xfd24:
    return "AUDIO0_BACKUP";
  case 0xfd25:
    return "AUDIO0_CONTROL";
  case 0xfd26:
    return "AUDIO0_COUNTER";
  case 0xfd27:
    return "AUDIO0_OTHER";
  case 0xfd28:
    return "AUDIO1_VOLCNTRL";
  case 0xfd29:
    return "AUDIO1_FEEDBACK";
  case 0xfd2a:
    return "AUDIO1_OUTPUT";
  case 0xfd2b:
    return "AUDIO1_SHIFT";
  case 0xfd2c:
    return "AUDIO1_BACKUP";
  case 0xfd2d:
    return "AUDIO1_CONTROL";
  case 0xfd2e:
    return "AUDIO1_COUNTER";
  case 0xfd2f:
    return "AUDIO1_OTHER";
  case 0xfd30:
    return "AUDIO2_VOLCNTRL";
  case 0xfd31:
    return "AUDIO2_FEEDBACK";
  case 0xfd32:
    return "AUDIO2_OUTPUT";
  case 0xfd33:
    return "AUDIO2_SHIFT";
  case 0xfd34:
    return "AUDIO2_BACKUP";
  case 0xfd35:
    return "AUDIO2_CONTROL";
  case 0xfd36:
    return "AUDIO2_COUNTER";
  case 0xfd37:
    return "AUDIO2_OTHER";
  case 0xfd38:
    return "AUDIO2_VOLCNTRL";
  case 0xfd39:
    return "AUDIO2_FEEDBACK";
  case 0xfd3a:
    return "AUDIO2_OUTPUT";
  case 0xfd3b:
    return "AUDIO2_SHIFT";
  case 0xfd3c:
    return "AUDIO2_BACKUP";
  case 0xfd3d:
    return "AUDIO2_CONTROL";
  case 0xfd3e:
    return "AUDIO2_COUNTER";
  case 0xfd3f:
    return "AUDIO2_OTHER";
  case 0xfd40:
    return "ATTENREG0";
  case 0xfd41:
    return "ATTENREG1";
  case 0xfd42:
    return "ATTENREG2";
  case 0xfd43:
    return "ATTENREG3";
  case 0xfd44:
    return "MPAN";
  case 0xfd50:
    return "STEREO";
  case 0xfd80:
    return "INTRST";
  case 0xfd81:
    return "INTSET";
  case 0xfd86:
    return "AUDIN";
  case 0xfd87:
    return "SYSCTL1";
  case 0xfd88:
    return "MIKEYHREV";
  case 0xfd89:
    return "MIKEYSREV";
  case 0xfd8a:
    return "IODIR";
  case 0xfd8b:
    return "IODAT";
  case 0xfd8c:
    return "SERCTL";
  case 0xfd8d:
    return "SERDAT";
  case 0xfd90:
    return "SDONEACK";
  case 0xfd91:
    return "CPUSLEEP";
  case 0xfd92:
    return "DISPCTL";
  case 0xfd93:
    return "PBKUP";
  case 0xfd94:
    return "DISPADR";
  case 0xfd95:
    return "DISPADR+1";
  case 0xfd9c:
    return "MTEST0";
  case 0xfd9d:
    return "MTEST1";
  case 0xfd9e:
    return "MTEST2";
  case 0xfda0:
    return "GREEN0";
  case 0xfda1:
    return "GREEN1";
  case 0xfda2:
    return "GREEN2";
  case 0xfda3:
    return "GREEN3";
  case 0xfda4:
    return "GREEN4";
  case 0xfda5:
    return "GREEN5";
  case 0xfda6:
    return "GREEN6";
  case 0xfda7:
    return "GREEN7";
  case 0xfda8:
    return "GREEN8";
  case 0xfda9:
    return "GREEN9";
  case 0xfdaa:
    return "GREENA";
  case 0xfdab:
    return "GREENB";
  case 0xfdac:
    return "GREENC";
  case 0xfdad:
    return "GREEND";
  case 0xfdae:
    return "GREENE";
  case 0xfdaf:
    return "GREENF";
  case 0xfdb0:
    return "BLUERED0";
  case 0xfdb1:
    return "BLUERED1";
  case 0xfdb2:
    return "BLUERED2";
  case 0xfdb3:
    return "BLUERED3";
  case 0xfdb4:
    return "BLUERED4";
  case 0xfdb5:
    return "BLUERED5";
  case 0xfdb6:
    return "BLUERED6";
  case 0xfdb7:
    return "BLUERED7";
  case 0xfdb8:
    return "BLUERED8";
  case 0xfdb9:
    return "BLUERED9";
  case 0xfdba:
    return "BLUEREDa";
  case 0xfdbb:
    return "BLUEREDb";
  case 0xfdbc:
    return "BLUEREDc";
  case 0xfdbd:
    return "BLUEREDd";
  case 0xfdbe:
    return "BLUEREDe";
  case 0xfdbf:
    return "BLUEREDf";
  case 0xfff9:
    return "MAPCTL";
  case 0xfffa:
    return "CPU_NMI";
  case 0xfffb:
    return "CPU_NMI+1";
  case 0xfffc:
    return "CPU_RESET";
  case 0xfffd:
    return "CPU_RESET+1";
  case 0xfffe:
    return "CPU_IRQ";
  case 0xffff:
    return "CPU_IRQ+1";


  default:
    if ( address < 256 )
    {
      sprintf( dest, "$%02x", (uint8_t)address );
    }
    else
    {
      sprintf( dest, "$%04x", address );
    }
    return dest;
  }
}

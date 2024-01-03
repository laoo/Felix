#include "pch.hpp"
#include "Monitor.hpp"
#include "Core.hpp"

void Monitor::addEntry( Entry entry )
{
  mEntries.push_back( std::move( entry ) );
}

cppcoro::generator<std::string_view> Monitor::sample( Core const& core )
{
  char buf[128];

  for ( auto const& e : mEntries )
  {
    char* pBuf = buf;
    switch ( e.type )
    {
    case Entry::Type::UNKNOWN:
      pBuf += sprintf( pBuf, "%s: not found", e.name.c_str() );
      co_yield std::string_view{ buf, ( size_t )( pBuf - buf ) };
      break;
    case Entry::Type::HEX:
      if ( e.size > 16 )
      {
        pBuf += sprintf( pBuf, "%s: size too large", e.name.c_str() );
        co_yield std::string_view{ buf, ( size_t )( pBuf - buf ) };
        break;
      }
      pBuf += sprintf( pBuf, "%s: $", e.name.c_str() );
      for ( int i = 0; i < e.size; ++i )
      {
        uint32_t value = core.debugReadRAM( e.address + i );
        pBuf += sprintf( pBuf, "%02x", value );
      }
      co_yield std::string_view{ buf, (size_t)( pBuf - buf ) };
      break;
    case Entry::Type::UNSIGNED:
      switch ( e.size )
      {
      case 1:
      {
        uint32_t value = core.debugReadRAM( e.address );
        pBuf += sprintf( pBuf, "%s: %u", e.name.c_str(), value );
        co_yield std::string_view{ buf, ( size_t )( pBuf - buf ) };
        break;
      }
      case 2:
      {
        uint32_t value = core.debugReadRAM( e.address );
        value |= ( uint64_t )core.debugReadRAM( e.address + 1 ) << 8;
        pBuf += sprintf( pBuf, "%s: %u", e.name.c_str(), value );
        co_yield std::string_view{ buf, ( size_t )( pBuf - buf ) };
        break;
      }
      case 3:
      {
        uint32_t value = core.debugReadRAM( e.address );
        value |= ( uint64_t )core.debugReadRAM( e.address + 1 ) << 8;
        value |= ( uint64_t )core.debugReadRAM( e.address + 2 ) << 16;
        pBuf += sprintf( pBuf, "%s: %u", e.name.c_str(), value );
        co_yield std::string_view{ buf, ( size_t )( pBuf - buf ) };
        break;
      }
      case 4:
      {
        uint32_t value = core.debugReadRAM( e.address );
        value |= ( uint64_t )core.debugReadRAM( e.address + 1 ) << 8;
        value |= ( uint64_t )core.debugReadRAM( e.address + 2 ) << 16;
        value |= ( uint64_t )core.debugReadRAM( e.address + 3 ) << 24;
        pBuf += sprintf( pBuf, "%s: %u", e.name.c_str(), value );
        co_yield std::string_view{ buf, ( size_t )( pBuf - buf ) };
        break;
      }
      default:
        break;
      }
      break;
    case Entry::Type::SIGNED:
      switch ( e.size )
      {
      case 1:
      {
        int8_t value = core.debugReadRAM( e.address );
        pBuf += sprintf( pBuf, "%s: %hhd", e.name.c_str(), value );
        co_yield std::string_view{ buf, ( size_t )( pBuf - buf ) };
        break;
      }
      case 2:
      {
        int16_t value = core.debugReadRAM( e.address );
        value |= ( int16_t )core.debugReadRAM( e.address + 1 ) << 8;
        pBuf += sprintf( pBuf, "%s: %hd", e.name.c_str(), value );
        co_yield std::string_view{ buf, ( size_t )( pBuf - buf ) };
        break;
      }
      case 4:
      {
        int32_t value = core.debugReadRAM( e.address );
        value |= ( int32_t )core.debugReadRAM( e.address + 1 ) << 8;
        value |= ( int32_t )core.debugReadRAM( e.address + 2 ) << 16;
        value |= ( int32_t )core.debugReadRAM( e.address + 3 ) << 24;
        pBuf += sprintf( pBuf, "%s: %d", e.name.c_str(), value );
        co_yield std::string_view{ buf, ( size_t )( pBuf - buf ) };
        break;
      }
      default:
        break;
      }
      break;
    default:
      break;
    }
  }
}



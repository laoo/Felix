#include "pch.hpp"
#include "WatchEditor.hpp"
#include "Manager.hpp"
#include "Core.hpp"
#include "Debugger.hpp"

WatchEditor::WatchEditor()
{
}

WatchEditor::~WatchEditor()
{
}

void WatchEditor::setManager( Manager* manager )
{
  mManager = manager;
}

bool WatchEditor::enabled()
{
  return mManager && mManager->mInstance && mManager->mDebugger.visualizeCPU;
}

bool WatchEditor::isReadOnly()
{
  if ( !enabled() )
  {
    return true;
  }

  return !mManager->mDebugger.isPaused();
}

const char* WatchEditor::dataTypeGetDesc( ImGuiDataType data_type ) const
{
  const char* descs[] = { "Int8", "Uint8", "Int16", "Uint16", "Int32", "Uint32", "Int64", "Uint64", "Float", "Double" };
  IM_ASSERT( data_type >= 0 && data_type < ImGuiDataType_COUNT );
  return descs[data_type];
}

ImGuiDataType WatchEditor::dataTypeGetType( const char* data_type )
{
  for ( int i = 0; i < ImGuiDataType_COUNT; ++i )
  {
    auto t = dataTypeGetDesc( i );
    if ( 0 == _stricmp( data_type, t ) )
    {
      return i;
    }
  }
  return -1;  
}

size_t WatchEditor::dataTypeGetSize( ImGuiDataType data_type ) const
{
  const size_t sizes[] = { 1, 1, 2, 2, 4, 4, 8, 8, sizeof( float ), sizeof( double ) };
  IM_ASSERT( data_type >= 0 && data_type < ImGuiDataType_COUNT );
  return sizes[data_type];
}

const char*WatchEditor::formatBinary( const uint8_t* buf, int width ) const
{
  IM_ASSERT( width <= 64 );
  size_t out_n = 0;
  static char out_buf[64 + 8 + 1];
  int n = width / 8;
  for ( int j = n - 1; j >= 0; --j )
  {
    for ( int i = 0; i < 8; ++i )
      out_buf[out_n++] = ( buf[j] & ( 1 << ( 7 - i ) ) ) ? '1' : '0';
    out_buf[out_n++] = ' ';
  }
  IM_ASSERT( out_n < IM_ARRAYSIZE( out_buf ) );
  out_buf[out_n] = 0;
  return out_buf;
}

void WatchEditor::drawPreviewData( const ImU8* mem_data, size_t mem_size, ImGuiDataType data_type, DataFormat data_format, char* out_buf, size_t out_buf_size ) const
{
  uint8_t buf[8];
  size_t size = dataTypeGetSize( data_type );
  
  memcpy( buf, mem_data, size );

  if ( data_format == DataFormat_Bin )
  {
    uint8_t binbuf[8];
    memcpy( binbuf, buf, size );
    snprintf( out_buf, out_buf_size, "%s", formatBinary( binbuf, (int)size * 8 ) );
    return;
  }

  out_buf[0] = 0;
  switch ( data_type )
  {
  case ImGuiDataType_S8:
  {
    int8_t int8 = 0;
    memcpy( &int8, buf, size );
    if ( data_format == DataFormat_Dec ) { snprintf( out_buf, out_buf_size, "%hhd", int8 ); return; }
    if ( data_format == DataFormat_Hex ) { snprintf( out_buf, out_buf_size, "0x%02x", int8 & 0xFF ); return; }
    break;
  }
  case ImGuiDataType_U8:
  {
    uint8_t uint8 = 0;
    memcpy( &uint8, buf, size );
    if ( data_format == DataFormat_Dec ) { snprintf( out_buf, out_buf_size, "%hhu", uint8 ); return; }
    if ( data_format == DataFormat_Hex ) { snprintf( out_buf, out_buf_size, "0x%02x", uint8 & 0XFF ); return; }
    break;
  }
  case ImGuiDataType_S16:
  {
    int16_t int16 = 0;
    memcpy( &int16, buf, size );
    if ( data_format == DataFormat_Dec ) { snprintf( out_buf, out_buf_size, "%hd", int16 ); return; }
    if ( data_format == DataFormat_Hex ) { snprintf( out_buf, out_buf_size, "0x%04x", int16 & 0xFFFF ); return; }
    break;
  }
  case ImGuiDataType_U16:
  {
    uint16_t uint16 = 0;
    memcpy( &uint16, buf, size );
    if ( data_format == DataFormat_Dec ) { snprintf( out_buf, out_buf_size, "%hu", uint16 ); return; }
    if ( data_format == DataFormat_Hex ) { snprintf( out_buf, out_buf_size, "0x%04x", uint16 & 0xFFFF ); return; }
    break;
  }
  case ImGuiDataType_S32:
  {
    int32_t int32 = 0;
    memcpy( &int32, buf, size );
    if ( data_format == DataFormat_Dec ) { snprintf( out_buf, out_buf_size, "%d", int32 ); return; }
    if ( data_format == DataFormat_Hex ) { snprintf( out_buf, out_buf_size, "0x%08x", int32 ); return; }
    break;
  }
  case ImGuiDataType_U32:
  {
    uint32_t uint32 = 0;
    memcpy( &uint32, buf, size );
    if ( data_format == DataFormat_Dec ) { snprintf( out_buf, out_buf_size, "%u", uint32 ); return; }
    if ( data_format == DataFormat_Hex ) { snprintf( out_buf, out_buf_size, "0x%08x", uint32 ); return; }
    break;
  }
  case ImGuiDataType_S64:
  {
    int64_t int64 = 0;
    memcpy( &int64, buf, size );
    if ( data_format == DataFormat_Dec ) { snprintf( out_buf, out_buf_size, "%lld", (long long)int64 ); return; }
    if ( data_format == DataFormat_Hex ) { snprintf( out_buf, out_buf_size, "0x%016llx", (long long)int64 ); return; }
    break;
  }
  case ImGuiDataType_U64:
  {
    uint64_t uint64 = 0;
    memcpy( &uint64, buf, size );
    if ( data_format == DataFormat_Dec ) { snprintf( out_buf, out_buf_size, "%llu", (long long)uint64 ); return; }
    if ( data_format == DataFormat_Hex ) { snprintf( out_buf, out_buf_size, "0x%016llx", (long long)uint64 ); return; }
    break;
  }
  case ImGuiDataType_Float:
  {
    float float32 = 0.0f;
    memcpy( &float32, buf, size );
    if ( data_format == DataFormat_Dec ) { snprintf( out_buf, out_buf_size, "%f", float32 ); return; }
    if ( data_format == DataFormat_Hex ) { snprintf( out_buf, out_buf_size, "%a", float32 ); return; }
    break;
  }
  case ImGuiDataType_Double:
  {
    double float64 = 0.0;
    memcpy( &float64, buf, size );
    if ( data_format == DataFormat_Dec ) { snprintf( out_buf, out_buf_size, "%f", float64 ); return; }
    if ( data_format == DataFormat_Hex ) { snprintf( out_buf, out_buf_size, "%a", float64 ); return; }
    break;
  }
  case ImGuiDataType_COUNT:
    break;
  } // Switch
  IM_ASSERT( 0 ); // Shouldn't reach
}

void WatchEditor::deleteWatch( const char* label )
{
  mItems.erase( std::remove_if( mItems.begin(), mItems.end(), [label] ( WatchItem x ) { return 0 == strcmp( x.label, label ); } ), mItems.end() );
}

void WatchEditor::deleteWatch( uint16_t id )
{
  mItems.erase( std::remove_if( mItems.begin(), mItems.end(), [id] ( WatchItem x ) { return x.id == id; } ), mItems.end() );
}

void WatchEditor::deleteWatch( const WatchItem* item )
{
  mItems.erase( std::remove( mItems.begin(), mItems.end(), *item ), mItems.end() );
}

void WatchEditor::addWatch( const char* label, ImGuiDataType type, const char* addr )
{
  if ( strlen( addr ) != 4 )
  {
    return;
  }

  char addrbuf[4] = { };

  for ( int i = 0; i < sizeof( addrbuf ); ++i )
  {
    int c = toupper( addr[i] );
    if ( c < '0' || c > 'F' || ( c > '9' && c < 'A' ) )
    {
      return;
    }
    addrbuf[i] = c;
  }

  addWatch( label, type, _4CHAR_TO_HEX( addr ) );
}

void WatchEditor::addWatch( const char* label, ImGuiDataType type, uint16_t addr )
{
  if ( strlen( label ) <= 0 || addr > 0xffff)
  {
    return;
  }

  WatchItem item;

  if ( !mItems.empty() )
  {
    item.id = mItems.back().id + 1;
  }
  item.type = type;
  strncpy( item.label, label, 17 );
  item.address = addr;

  mItems.push_back( item );
}

void WatchEditor::addWatch( const char* label, const char* type, uint16_t addr )
{
  ImGuiDataType t = dataTypeGetType( type );

  if ( t < 0 )
  {
    return;
  }

  addWatch( label, t, addr );
}

void WatchEditor::drawContents()
{
  WatchItem item{ };

  ImGui::AlignTextToFramePadding();

  ImGui::Text("Label");
  
  ImGui::SameLine();
  ImGui::SetNextItemWidth( 120 );
  ImGui::InputText( "##watchlabel", mNewItemLabelBuf, 16 );
  
  ImGui::SameLine();
  ImGui::Text( "Address" );

  ImGui::SameLine();
  ImGui::SetNextItemWidth( 35 );
  auto addrflag = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase;
  ImGui::InputText( "##watchaddr", mNewItemAddrBuf, 5, addrflag );
  
  ImGui::SameLine();
  ImGui::Text( "Type" );

  ImGui::SameLine();
  ImGui::SetNextItemWidth( 70 );
  if ( ImGui::BeginCombo( "##watchtype", dataTypeGetDesc( mNewItemDataType ), ImGuiComboFlags_HeightLargest ) )
  {
    for ( int n = 0; n < ImGuiDataType_COUNT; n++ )
      if ( ImGui::Selectable( dataTypeGetDesc( (ImGuiDataType)n ), mNewItemDataType == n ) )
        mNewItemDataType = (ImGuiDataType)n;
    ImGui::EndCombo();
  }

  ImGui::SameLine();
  ImGui::SetNextItemWidth( 50 );
  if ( ImGui::Button( "Add" ) )
  {
    if ( strlen( mNewItemLabelBuf ) <= 0 || strlen( mNewItemAddrBuf ) != 4 || isReadOnly() )
    {    
      return;
    }

    addWatch( mNewItemLabelBuf, mNewItemDataType, mNewItemAddrBuf );
  }

  ImGui::BeginTable( "##watchitems", 6, ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit );
  ImGui::TableSetupColumn( "Del" );
  ImGui::TableSetupColumn( "Label" );
  ImGui::TableSetupColumn( "Addr" );
  ImGui::TableSetupColumn( "Hex" );
  ImGui::TableSetupColumn( "Dec" );
  ImGui::TableSetupColumn( "Bin" );
  ImGui::TableHeadersRow();
  for ( const auto& item : mItems ) 
  {
    auto size = dataTypeGetSize( item.type );

    do
    {
      --size;
      mDataBuf[size] = mManager->mInstance->debugReadRAM( item.address + (uint16_t)size );
    } while ( size > 0 );

    sprintf( mLabelBuf, "##wi%d", item.id );
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth( 15 );
    if ( ImGui::ColorButton( mLabelBuf, ImVec4(255, 0, 0, 0), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs ) )
    {
      deleteWatch( &item );
    }

    ImGui::TableNextColumn();
    ImGui::Text( item.label );

    ImGui::TableNextColumn();
    snprintf( mDataOutputBuf, 7, "0x%04X", item.address );
    ImGui::Text( mDataOutputBuf );

    ImGui::TableNextColumn();
    drawPreviewData( mDataBuf, sizeof( mDataBuf ), item.type, DataFormat_Hex, mDataOutputBuf, sizeof( mDataOutputBuf ) );
    ImGui::Text( mDataOutputBuf );
    
    ImGui::TableNextColumn();
    drawPreviewData( mDataBuf, sizeof( mDataBuf ), item.type, DataFormat_Dec, mDataOutputBuf, sizeof( mDataOutputBuf ) );
    ImGui::Text( mDataOutputBuf );

    ImGui::TableNextColumn();
    drawPreviewData( mDataBuf, sizeof( mDataBuf ), item.type, DataFormat_Bin, mDataOutputBuf, sizeof( mDataOutputBuf ) );
    ImGui::Text( mDataOutputBuf );
  }
  ImGui::EndTable();

}
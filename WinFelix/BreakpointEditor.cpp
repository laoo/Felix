#include "pch.hpp"
#include "BreakpointEditor.hpp"
#include "Manager.hpp"
#include "Core.hpp"
#include "Debugger.hpp"

BreakpointEditor::BreakpointEditor()
{
}

BreakpointEditor::~BreakpointEditor()
{
}

void BreakpointEditor::setManager( Manager* manager )
{
  mManager = manager;
}

bool BreakpointEditor::enabled()
{
  return mManager && mManager->mInstance && mManager->mDebugger.visualizeBreakpoint;
}

bool BreakpointEditor::isReadOnly()
{
  if ( !enabled() )
  {
    return true;
  }

  return !mManager->mDebugger.isPaused();
}

void BreakpointEditor::deleteBreakpoint( const BreakpointItem* item )
{
  mItems.erase( std::remove( mItems.begin(), mItems.end(), *item ), mItems.end() );
}

void BreakpointEditor::addBreakpoint( BreakpointType type, const char* addr )
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

  addBreakpoint( type, _4CHAR_TO_HEX( addr ) );
}

void BreakpointEditor::addBreakpoint( BreakpointType type, uint16_t addr )
{
  if ( addr > 0xffff )
  {
    return;
  }

  BreakpointItem item;

  if ( !mItems.empty() )
  {
    item.id = mItems.back().id + 1;
  }
  item.type = type;
  item.address = addr;

  mItems.push_back( item );
}

const char* BreakpointEditor::breakpointTypeGetDesc( BreakpointType type ) const
{
  const char* descs[] = { "Exec", "Read", "Write" };
  IM_ASSERT( type >= 0 && type < BreakpointType_COUNT );
  return descs[type];
}

void BreakpointEditor::drawContents()
{
  WatchItem item{ };
  char idBuf[10]{ };

  ImGui::AlignTextToFramePadding();

  ImGui::Text( "Address" );

  ImGui::SameLine();
  ImGui::SetNextItemWidth( 35 );
  auto addrflag = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase;
  ImGui::InputText( "##watchaddr", mNewItemAddrBuf, 5, addrflag );

  ImGui::SameLine();
  ImGui::Text( "Type" );

  ImGui::SameLine();
  ImGui::SetNextItemWidth( 70 );
  if ( ImGui::BeginCombo( "##breakPointtype", breakpointTypeGetDesc( mNewItemBreakpointType ), ImGuiComboFlags_HeightLargest ) )
  {
    for ( int n = 0; n < BreakpointType_COUNT; n++ )
      if ( ImGui::Selectable( breakpointTypeGetDesc( (BreakpointType)n ), mNewItemBreakpointType == n ) )
        mNewItemBreakpointType = (BreakpointType)n;
    ImGui::EndCombo();
  }

  ImGui::SameLine();
  ImGui::SetNextItemWidth( 50 );
  if ( ImGui::Button( "Add" ) )
  {
    if ( strlen( mNewItemAddrBuf ) != 4 || isReadOnly() )
    {
      return;
    }

    addBreakpoint( mNewItemBreakpointType, mNewItemAddrBuf );
  }

  ImGui::Separator();

  if ( ImGui::BeginTable( "##bpitems", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit ) )
  {
    ImGui::TableSetupColumn( "Del" );
    ImGui::TableSetupColumn( "Address" );
    ImGui::TableSetupColumn( "Type" );
    ImGui::TableSetupScrollFreeze( 0, 1 );
    ImGui::TableHeadersRow();

    for ( const auto& item : mItems )
    {
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth( 15 );
      sprintf( idBuf, "##bi%d", item.id );
      if ( ImGui::ColorButton( idBuf, ImVec4( 255, 0, 0, 0 ), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs ) )
      {
        deleteBreakpoint( &item );
      }

      ImGui::TableNextColumn();
      //snprintf( mDataOutputBuf, 7, "0x%04X", item.address );
      ImGui::Text( "mDataOutputBuf" );

      ImGui::TableNextColumn();
      //drawPreviewData( mDataBuf, sizeof( mDataBuf ), item.type, DataFormat_Hex, mDataOutputBuf, sizeof( mDataOutputBuf ) );
      ImGui::Text( "mDataOutputBuf" );
    }
    ImGui::EndTable();
  }
}
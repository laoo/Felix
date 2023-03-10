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

  if ( enabled() )
  {
    initializeExistingTraps();
  }
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

bool BreakpointEditor::hasBreapoint( uint16_t address )
{
  return std::any_of( mItems.begin(), mItems.end(), [address] ( BreakpointItem i ) { return i.address == address; } );
}

void BreakpointEditor::toggleBreapoint( uint16_t address )
{
  if ( hasBreapoint( address ) )
  {
    deleteBreakpoint( address );
  }
  else
  {
    addBreakpoint( ScriptDebugger::Type::RAM_EXECUTE, address );
  }
}

void BreakpointEditor::initializeExistingTraps()
{
  auto traps = mManager->mInstance->getScriptDebugger()->getTraps( IMemoryAccessTrap::UI );

  for ( auto trap : traps )
  {
    BreakpointItem item;

    item.type = std::get<0>( trap );
    item.address = std::get<1>( trap );

    mItems.push_back( item );
  }
}

void BreakpointEditor::deleteBreakpoint( uint16_t address )
{
  auto iter = std::find_if( mItems.begin(), mItems.end(), [address] ( BreakpointItem item ) { return item.address == address && item.type == ScriptDebugger::Type::RAM_EXECUTE; } );

  if ( iter == mItems.end() )
  {
    return;
  }

  deleteBreakpoint( &(*iter) );
}

void BreakpointEditor::deleteBreakpoint( const BreakpointItem* item )
{
  mManager->mInstance->getScriptDebugger()->deleteTrap( item->type, item->address );
  mItems.erase( std::remove( mItems.begin(), mItems.end(), *item ), mItems.end() );
}

void BreakpointEditor::addBreakpoint( ScriptDebugger::Type type, const char* addr )
{
  int v;
  sscanf( addr, "%04X", &v );
  addBreakpoint( type, v );
}

void BreakpointEditor::addBreakpoint( ScriptDebugger::Type type, uint16_t addr )
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

  auto trap = std::make_shared<UIBreakpointTrap>();

  mManager->mInstance->getScriptDebugger()->addTrap( item.type, item.address, trap);
  mItems.push_back( item );
}

const char* BreakpointEditor::breakpointTypeGetDesc( ScriptDebugger::Type type ) const
{
  const char* descs[] = { "RAM read", "RAM Write", "RAM Execute", "ROM Read", "ROM Write", "ROM Execute", "Mikey Read", "Mikey Write", "Suzy Read", "Suzy Write" };
  IM_ASSERT( (int)type >= 0 && type < ScriptDebugger::Type::MAPCTL_READ );
  return descs[(int)type];
}

void BreakpointEditor::drawContents()
{
  WatchItem item{ };
  char buffer[50]{ };

  ImGui::AlignTextToFramePadding();

  ImGui::Text( "Address" );

  ImGui::SameLine();
  ImGui::SetNextItemWidth( 35 );
  auto addrflag = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase;
  ImGui::InputText( "##watchaddr", mNewItemAddrBuf, 5, addrflag );

  ImGui::SameLine();
  ImGui::Text( "Type" );

  ImGui::SameLine();
  ImGui::SetNextItemWidth( 100 );
  if ( ImGui::BeginCombo( "##breakPointtype", breakpointTypeGetDesc( mNewItemBreakpointType ), ImGuiComboFlags_HeightLargest ) )
  {
    for ( int n = 0; n < (int)ScriptDebugger::Type::MAPCTL_READ; n++ )
      if ( ImGui::Selectable( breakpointTypeGetDesc( (ScriptDebugger::Type)n ), (int)mNewItemBreakpointType == n ) )
        mNewItemBreakpointType = (ScriptDebugger::Type)n;
    ImGui::EndCombo();
  }

  ImGui::SameLine();
  ImGui::SetNextItemWidth( 50 );
  if ( !isReadOnly() && ImGui::Button( "Add" ) )
  {
    if ( strlen( mNewItemAddrBuf ) <= 0 || isReadOnly() )
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
      sprintf( buffer, "##bi%d", item.id );
      if ( ImGui::ColorButton( buffer, ImVec4( 255, 0, 0, 0 ), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs ) )
      {
        deleteBreakpoint( &item );
      }

      ImGui::TableNextColumn();
      snprintf( buffer, 7, "$%04X", item.address );
      ImGui::Text( buffer );

      ImGui::TableNextColumn();
      ImGui::Text( breakpointTypeGetDesc( item.type ) );
    }
    ImGui::EndTable();
  }
}
#pragma once

#include "Editors.hpp"

class Manager;

typedef struct WatchItem
{
  uint32_t id = 0;
  char label[17];
  ImGuiDataType type = ImGuiDataType_U8;
  uint16_t address = 0;

  bool operator==( const WatchItem& b )
  {
    return id == b.id;
  }

} WatchItem;

enum DataFormat
{
  DataFormat_Bin = 0,
  DataFormat_Dec = 1,
  DataFormat_Hex = 2,
  DataFormat_COUNT
};

class WatchEditor
{
public:
  WatchEditor();
  ~WatchEditor();

  void setManager( Manager* manager );
  void drawContents();
  bool enabled();

  void deleteWatch( const char* label );
  void addWatch( const char* label, const char* type, uint16_t addr );

private:
  Manager* mManager;
  std::vector<WatchItem> mItems;

  char mNewItemLabelBuf[17];
  char mNewItemAddrBuf[6];
  ImGuiDataType mNewItemDataType;
  ImU8 mDataBuf[8];
  char mDataOutputBuf[8 * 8];
  char mLabelBuf[10];

  bool isReadOnly();

  void deleteWatch( const WatchItem* item );
  void deleteWatch( uint16_t id );
  void addWatch( const char* label, ImGuiDataType type, const char* addr );
  void addWatch( const char* label, ImGuiDataType type, uint16_t addr );
  ImGuiDataType dataTypeGetType( const char* data_type );
  const char* dataTypeGetDesc( ImGuiDataType data_type ) const;
  size_t dataTypeGetSize( ImGuiDataType data_type ) const;
  void drawPreviewData( const ImU8* mem_data, size_t mem_size, ImGuiDataType data_type, DataFormat data_format, char* out_buf, size_t out_buf_size ) const;
  const char* formatBinary( const uint8_t* buf, int width ) const;
};


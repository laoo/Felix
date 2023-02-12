#pragma once

#include "imgui.h"
#include <cstdint>

#define REG_TXT_LEN 3
#define PS_TXT_LEN  5
#define ITEM_WIDTH  35

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 


class Manager;

class CPUEditor
{
public:
  CPUEditor();

  void setManager( Manager* manager );
  void drawContents();
  bool enabled();

private:

  Manager* mManager;
  char mA[REG_TXT_LEN] = {};
  char mX[REG_TXT_LEN] = {};
  char mY[REG_TXT_LEN] = {};
  char mP[PS_TXT_LEN] = {};
  char mS[PS_TXT_LEN] = {};
  bool mN = { false };
  bool mV = { false };
  bool mD = { false };
  bool mI = { false };
  bool mZ = { false };
  bool mC = { false };

  void drawRegister( const char* label, uint8_t reg, char* reg_buf );
  void drawPS( const char* label, uint16_t ps, char* ps_buf );
  void drawFlag( const char* label, bool enabled, bool* b );
  bool isReadOnly();
};
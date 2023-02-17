#pragma once

#include "Editors.hpp"

#define REG_TXT_LEN 3
#define PS_TXT_LEN  5
#define ITEM_WIDTH  35
#define LABEL_WIDTH 16

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
  char mPC[PS_TXT_LEN] = {};
  char mS[PS_TXT_LEN] = {};
  bool mN = { false };
  bool mV = { false };
  bool mD = { false };
  bool mI = { false };
  bool mZ = { false };
  bool mC = { false };

  char mlabel_buf[10];

  void drawRegister( const char* label, uint8_t reg, char* reg_buf );
  void drawPS( const char* label, uint16_t ps, char* ps_buf );
  void drawFlag( const char* label, bool enabled, bool* b );
  bool isReadOnly();
};
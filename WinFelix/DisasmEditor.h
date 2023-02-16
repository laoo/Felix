#pragma once

#pragma once

#include "Editors.hpp"

#define REG_TXT_LEN 3
#define PS_TXT_LEN  5
#define ITEM_WIDTH  35
#define LABEL_WIDTH 16

class Manager;

class DisasmEditor
{
public:
  DisasmEditor();
  ~DisasmEditor();

  void setManager( Manager* manager );
  void drawContents();
  bool enabled();

private:

  Manager* mManager;
  int mPC;
  int mTablePC;
  bool mFollowPC;
  bool mShowLabelsInAddrCol;

  bool isReadOnly();

  void drawTable();
  void drawOptions();
  void scrollUp();
  void scrollDown();
};


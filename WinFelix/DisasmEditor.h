#pragma once

#include "Editors.hpp"

class Manager;

class DisasmEditor : public IEditor
{
public:
  DisasmEditor();
  ~DisasmEditor() override;

  void drawContents() override;
  bool enabled() override;

  void coreHasBeenReset() override;

private:
  
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


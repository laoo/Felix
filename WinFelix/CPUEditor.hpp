#pragma once

#include "Editors.hpp"

class Manager;

class CPUEditor : public IEditor
{
public:
  CPUEditor();

  void drawContents() override;
  bool enabled() override;

  void coreHasBeenReset() override;

private:

  bool mN = { false };
  bool mV = { false };
  bool mD = { false };
  bool mI = { false };
  bool mZ = { false };
  bool mC = { false };

  void drawRegister( const char* label, uint8_t reg );
  void drawPS( const char* label, uint16_t ps );
  void drawFlag( const char* label, bool enabled, bool* b );
  bool isReadOnly();
};

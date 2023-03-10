#pragma once

#include "imgui.h"
#include <cstdint>

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

#define CHAR_TO_HEX(c) ((c >= 'A')? (c - 'A' + 10): (c - '0'))
#define _2CHAR_TO_HEX(buf) ( (CHAR_TO_HEX(buf[0]) << 4 ) | CHAR_TO_HEX(buf[1]) )
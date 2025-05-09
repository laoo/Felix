## 0.6.4
- more accurate opcode/operand fetch timing
- making bootrom HLE emulation more compatible
- new feature: sprite dumper (Debug->Sprite Dump, and 'dumpSprites' lua command)
- corrected default values for the attenuation and panning registers.
- adding lua libraries of string, os, math and table 
- stability fixes

## 0.6.3

- icon file as a courtesy of https://retroemu.pl/
- added a command to dump emulated Lynx memory to a file
- added a command to save currently rendered frame to a PNG file
- removing buggy watch editor and breakpoint editor
- fixing a script typo (traceOf -> traceOff)
- making trace log set in lua script relative to the script file, not an emulator executable

## 0.6.2

- Loading symbols from .lab files generated by mads
- Ability to access symbols through Lua
- Added monitor window
- Fixed handling of lua errors

## 0.6.1

- Fixed long-lasting bug that resulted in too fast emulation and audio glitches (#109)
- Added WAV and VGM dumping commands to Audio menu

## 0.6.0

Yay!

A new Felix release after two years and ten days!

Many changes and improvements with respect to debugging and stability.

Known issues:

- Awesome golf and Scrapyard Dog hangs,
- audio/video synchronization has a defect resulting in crackling,
- few debugging features does not behave as flawlessly as they should.

This is the last (major-ish) release before much anticipated migration of the rendering engine to something multiplatform (likely SDL2).

## 0.5.2 BS42

 - Fix disassembly of BRK

## 0.5.1 BS42

 - add Ctrl-O for "open"
 - add Ctrl-R to re-open last file
 - add support to ignore "brk #$42" even if Break on BRK is off
 - move RAM Execute breakpoint to the top

## 0.5.0

- Introducing simple GUI
- Fixed XOR sprites drawing
- Redefinable key mapping
- Simple gamepad support
- Automatic key remapping depending on rotation
- Giving Felix the focus for keyboard after a drag&drop
- Cpu corrections (fixes Dracula)
- Ability to change rotation and eeprom properies (can't be saved yet)
- Extended lua debugging facilities

## 0.4.8

- DX9 Renderer

## 0.4.7

- Fixed many games hanging after title screen
- Got rid of D3DCompiler library dependency (might fix some DXError problems)

## 0.4.6

- Fixed math bug related to unsafe register access (demo0006.o now works)
- Fixed bug in counters
- Slightly improved CPU timing
- Fixed multiple errors related to hardware collisions (fixes Flappy Bird and other)

## 0.4.5

- EEPROM support reworked. Fixed bad write status

## 0.4.4

- support for rotation flag in LNX header

## 0.4.3

- single instance feature can be disabled in configuration file in `c:\Users\[user-name]\AppData\Local\Felix\WinConfig.lua` and is disabled by default
- fixed a math bug that resolves the issue with apfel_2a.o

## 0.4.2

- Suzy math corrections
- Palette corrections
- EEPROM support

## 0.4.1

This is a first public release.
Keep in mind that it's inherently "pre-release" and isn't production ready before reaching version 1.0

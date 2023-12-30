cmake_minimum_required (VERSION 3.25)

project ( Felix )

set( CMAKE_CXX_STANDARD 20 )

add_executable( Felix WIN32
  WinFelix/ConfigProvider.cpp
  WinFelix/ConfigProvider.hpp
  WinFelix/CPUEditor.cpp
  WinFelix/Debugger.cpp
  WinFelix/Debugger.hpp
  WinFelix/DX11Helpers.cpp
  WinFelix/DX11Helpers.hpp
  WinFelix/DX11Renderer.cpp
  WinFelix/DX11Renderer.hpp
  WinFelix/Ex.hpp
  WinFelix/ISystemDriver.hpp
  WinFelix/IUserInput.hpp
  WinFelix/KeyNames.cpp
  WinFelix/KeyNames.hpp
  WinFelix/LuaProxies.cpp
  WinFelix/LuaProxies.hpp
  WinFelix/Manager.cpp
  WinFelix/Manager.hpp
  WinFelix/rational.hpp
  WinFelix/Renderer.hpp
  WinFelix/ScreenGeometry.cpp
  WinFelix/ScreenGeometry.hpp
  WinFelix/ScreenRenderingBuffer.cpp
  WinFelix/ScreenRenderingBuffer.hpp
  WinFelix/SysConfig.cpp
  WinFelix/SysConfig.hpp
  WinFelix/SystemDriver.cpp
  WinFelix/SystemDriver.hpp
  WinFelix/UI.cpp
  WinFelix/UI.hpp
  WinFelix/UserInput.cpp
  WinFelix/UserInput.hpp
  WinFelix/VideoSink.cpp
  WinFelix/VideoSink.hpp
  WinFelix/WinAudioOut.cpp
  WinFelix/WinAudioOut.hpp
  WinFelix/WinImgui.cpp
  WinFelix/WinImgui.hpp
  WinFelix/WinImgui11.cpp
  WinFelix/WinImgui11.hpp
  WinFelix/WinMain.cpp

  WinFelix/BreakpointEditor.cpp
  WinFelix/BreakpointEditor.hpp
  WinFelix/CPUEditor.hpp
  WinFelix/DisasmEditor.cpp
  WinFelix/DisasmEditor.h
  WinFelix/Editors.hpp
  WinFelix/MemEditor.cpp
  WinFelix/MemEditor.hpp
  WinFelix/WatchEditor.cpp
  WinFelix/WatchEditor.hpp

  WinFelix/pixel.hxx
  WinFelix/renderer.hxx
  WinFelix/vertex.hxx

  libFelix/ActionQueue.cpp
  libFelix/ActionQueue.hpp
  libFelix/AudioChannel.cpp
  libFelix/AudioChannel.hpp
  libFelix/BootROMTraps.cpp
  libFelix/BootROMTraps.hpp
  libFelix/CartBank.cpp
  libFelix/CartBank.hpp
  libFelix/Cartridge.cpp
  libFelix/Cartridge.hpp
  libFelix/ColOperator.cpp
  libFelix/ColOperator.hpp
  libFelix/ComLynx.cpp
  libFelix/ComLynx.hpp
  libFelix/ComLynxWire.hpp
  libFelix/Core.cpp
  libFelix/Core.hpp
  libFelix/CPU.cpp
  libFelix/CPU.hpp
  libFelix/CPUState.cpp
  libFelix/CPUState.hpp
  libFelix/DebugRAM.hpp
  libFelix/DisplayGenerator.cpp
  libFelix/DisplayGenerator.hpp
  libFelix/EEPROM.cpp
  libFelix/EEPROM.hpp
  libFelix/Encryption.cpp
  libFelix/Encryption.hpp
  libFelix/GameDrive.cpp
  libFelix/GameDrive.hpp
  libFelix/generator.hpp
  libFelix/IInputSource.hpp
  libFelix/ImageBS93.cpp
  libFelix/ImageBS93.hpp
  libFelix/ImageCart.cpp
  libFelix/ImageCart.hpp
  libFelix/ImageProperties.cpp
  libFelix/ImageProperties.hpp
  libFelix/ImageROM.cpp
  libFelix/ImageROM.hpp
  libFelix/IMemoryAccessTrap.hpp
  libFelix/InputFile.cpp
  libFelix/InputFile.hpp
  libFelix/IVideoSink.hpp
  libFelix/Log.cpp
  libFelix/Log.hpp
  libFelix/Mikey.cpp
  libFelix/Mikey.hpp
  libFelix/Opcodes.hpp
  libFelix/ParallelPort.cpp
  libFelix/ParallelPort.hpp
  libFelix/ScriptDebugger.hpp
  libFelix/ScriptDebuggerEscapes.hpp
  libFelix/Shifter.hpp
  libFelix/SpriteLineParser.hpp
  libFelix/SpriteTemplates.hpp
  libFelix/Suzy.cpp
  libFelix/Suzy.hpp
  libFelix/SuzyMath.cpp
  libFelix/SuzyMath.hpp
  libFelix/SuzyProcess.cpp
  libFelix/SuzyProcess.hpp
  libFelix/SymbolSource.cpp
  libFelix/SymbolSource.hpp
  libFelix/TimerCore.cpp
  libFelix/TimerCore.hpp
  libFelix/TraceHelper.cpp
  libFelix/TraceHelper.hpp
  libFelix/Utility.cpp
  libFelix/Utility.hpp
  libFelix/VGMWriter.cpp
  libFelix/VGMWriter.hpp
  libFelix/VidOperator.cpp
  libFelix/VidOperator.hpp
)

include( cmake/version.cmake )
configure_file( WinFelix/version.hpp.in WinFelix/version.hpp @ONLY )
target_include_directories( Felix PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/WinFelix" )

target_include_directories( Felix PRIVATE libFelix )
target_include_directories( Felix PRIVATE Encoder/API )
target_include_directories( Felix PRIVATE libextern/sol2/include )
target_include_directories( Felix PRIVATE libextern/lua )
target_include_directories( Felix PRIVATE libextern/imgui )
target_include_directories( Felix PRIVATE libextern/imgui_club )
target_include_directories( Felix PRIVATE libextern/imgui-filebrowser )
target_include_directories( Felix PRIVATE libextern/libwav/include )
target_include_directories( Felix PRIVATE libextern/fmt/include )
target_include_directories( Felix PRIVATE libextern/multiprecision/include )

if (WIN32)
  target_compile_definitions(Felix PRIVATE -D_CRT_SECURE_NO_WARNINGS)
  target_compile_definitions(Felix PRIVATE -D_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS)
  target_compile_definitions(Felix PRIVATE -D_UNICODE)
  target_compile_definitions(Felix PRIVATE -DUNICODE)
endif()
target_compile_definitions(Felix PRIVATE -DAPP_NAME=\"${PROJECT_NAME}\")

target_precompile_headers( Felix PRIVATE
  <algorithm>
  <array>
  <atomic>
  <bit>
  <cassert>
  <charconv>
  <chrono>
  <concepts>
  <coroutine>
  <cstdint>
  <cwchar>
  <filesystem>
  <fstream>
  <functional>
  <initializer_list>
  <limits>
  <memory>
  <mutex>
  <optional>
  <ranges>
  <queue>
  <random>
  <span>
  <string>
  <thread>
  <stdexcept>
  <unordered_map>
  <utility>
  <vector>
  WinFelix/winpch.hpp
)

add_subdirectory( libextern )

target_link_libraries( Felix
  PRIVATE lua wav imgui
)

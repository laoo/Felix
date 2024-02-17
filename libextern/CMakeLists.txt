
add_library( lua STATIC
  lua/lapi.c
  lua/lctype.c
  lua/lfunc.c
  lua/lmathlib.c
  lua/loslib.c
  lua/ltable.c
  lua/lundump.c
  lua/lauxlib.c
  lua/ldblib.c
  lua/lgc.c
  lua/lmem.c
  lua/lparser.c
  lua/ltablib.c
  lua/lutf8lib.c
  lua/lbaselib.c
  lua/ldebug.c
  lua/linit.c
  lua/loadlib.c
  lua/lstate.c
  lua/ltests.c
  lua/lvm.c
  lua/lcode.c
  lua/ldo.c
  lua/liolib.c
  lua/lobject.c
  lua/lstring.c
  lua/ltm.c
  lua/lzio.c
  lua/lcorolib.c
  lua/ldump.c
  lua/llex.c
  lua/lopcodes.c
  lua/lstrlib.c
)

add_library( wav STATIC
  libwav/src/wav.c
)

target_include_directories( wav PRIVATE libwav/include )
target_compile_definitions( wav PRIVATE -D__LITTLE_ENDIAN__)

add_library( imgui STATIC
  imgui/imgui.cpp
  imgui/imgui_demo.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp
)

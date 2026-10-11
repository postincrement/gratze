set(NFD_SOURCES
  ${GRATZE_NFD_SOURCE}/src/nfd_common.c
)

if(APPLE)
  list(APPEND NFD_SOURCES ${GRATZE_NFD_SOURCE}/src/nfd_cocoa.m)
elseif(WIN32)
  list(APPEND NFD_SOURCES ${GRATZE_NFD_SOURCE}/src/nfd_win.cpp)
else()
  list(APPEND NFD_SOURCES ${GRATZE_NFD_SOURCE}/src/nfd_gtk.c)
endif()

add_library(nfd STATIC ${NFD_SOURCES})
target_include_directories(nfd PUBLIC ${GRATZE_NFD_SOURCE}/src/include)

if(APPLE)
  target_link_libraries(nfd PUBLIC "-framework Foundation" "-framework AppKit")
elseif(WIN32)
  target_link_libraries(nfd PUBLIC comdlg32 ole32 uuid shell32)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
  target_link_libraries(nfd PUBLIC PkgConfig::GTK3)
endif()

add_library(sdl_fontcache STATIC ${GRATZE_FONTCACHE_SOURCE}/SDL_FontCache.c)
target_link_libraries(sdl_fontcache PUBLIC ${GRATZE_SDL2} ${GRATZE_SDL2_TTF})
target_include_directories(sdl_fontcache PUBLIC ${GRATZE_GEN_INCLUDE})

find_program(Z80ASM z80asm REQUIRED)

set(NEWBDOS_ROM ${CMAKE_CURRENT_BINARY_DIR}/newbdos_rom.c)
add_custom_command(
  OUTPUT ${NEWBDOS_ROM}
  COMMAND ${CMAKE_COMMAND}
          -DZ80ASM=${Z80ASM}
          -DASM=${CMAKE_CURRENT_SOURCE_DIR}/z80/cpmhost/newbdos.asm
          -DOUT=${NEWBDOS_ROM}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/gen_newbdos_rom.cmake
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/z80/cpmhost/newbdos.asm
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/gen_newbdos_rom.cmake
  VERBATIM
)

add_executable(gratze
  src/main.cc
  src/mainwindow.cc
  src/status_panel.cc
  src/options.cc
  src/env_config.cc
  src/emulator.cc
  devices/fdc.cc
  devices/keyscan.cc
  devices/keypar.cc
  devices/z80pio.cc
  devices/intel8255.cc
  devices/synertek6545.cc
  video/virtual_screen.cc
  video/font.cc
  video/dg640.cc
  video/chargen_2513.cc
  video/chargen_mcm6574.cc
  video/chargen_mcm6674.cc
  terminal/terminal.cc
  terminal/noca_mode.cc
  z80/z80emulator.cc
  z80/trs80/trs80.cc
  z80/trs80/model1/model1.cc
  z80/trs80/model3/model3.cc
  z80/trs80/model3/chargen_model3.cc
  z80/trs80/model4/model4.cc
  z80/microbee/microbee.cc
  z80/microbee/mbee_font.cc
  z80/sorcerer/sorcerer.cc
  z80/sorcerer/chargen_sorcerer.cc
  z80/dg680/dg680.cc
  z80/super80/super80.cc
  z80/cpm80/cpm80.cc
  z80/cpmhost/newbdos.cc
  2650/2650emulator.cc
  2650/cpu_2650.cc
  2650/eti685/eti685.cc
  2650/78up5/78up5.cc
  z80/trs80/model1/rom_level1.c
  z80/trs80/model1/rom_level2.c
  z80/trs80/model3/rom_model3.c
  z80/dg680/rom_dgos1_4.c
  z80/super80/rom_super80.c
  z80/microbee/rom_basic5_22e.c
  z80/microbee/rom_disk56.c
  z80/microbee/rom_disk128.c
  z80/microbee/rom_bn5443.c
  z80/microbee/starnet/starnet_client.cc
  z80/sorcerer/rom_sorcerer.c
  z80/mfz80/Z80.c
  z80/mfz80/Debug.c
  z80/mfz80/dasm.c
  ${CMAKE_CURRENT_BINARY_DIR}/newbdos_rom.c
  2650/eti685/rom_binbug_6_1.c
  2650/78up5/rom_pipbug.c
  nfd_compat/nfd_ext.c
)

target_include_directories(gratze PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/nfd_compat
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/starnet
  ${GRATZE_GEN_INCLUDE}
)

target_compile_definitions(gratze PRIVATE EXECZ80)

if(CMAKE_CXX_BYTE_ORDER STREQUAL "BIG_ENDIAN")
  target_compile_definitions(gratze PRIVATE MSB_FIRST)
elseif(CMAKE_CXX_BYTE_ORDER STREQUAL "LITTLE_ENDIAN")
  target_compile_definitions(gratze PRIVATE LSB_FIRST)
else()
  message(FATAL_ERROR "Unknown target byte order '${CMAKE_CXX_BYTE_ORDER}'")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  target_compile_options(gratze PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-register>)
endif()

if(APPLE)
  target_sources(gratze PRIVATE src/mainwindow_mac.mm)
endif()

target_link_libraries(gratze PRIVATE
  grzdisk
  cpmdrive
  grzcommon
  starnet_udp
  nfd
  sdl_fontcache
  ${GRATZE_SDL2}
  ${GRATZE_SDL2_TTF}
)

if(NOT WIN32)
  find_package(Curses REQUIRED)
  target_include_directories(gratze PRIVATE ${CURSES_INCLUDE_DIR})
  target_link_libraries(gratze PRIVATE ${CURSES_LIBRARIES})
endif()

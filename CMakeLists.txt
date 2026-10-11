cmake_minimum_required(VERSION 3.20)

project(gratze VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
  enable_language(OBJC)
endif()

include(FetchContent)

# Populate these repositories without adding their own CMake projects.
# Cereal's project builds its tests, and SDL_FontCache's project is not how
# gratze compiles that single source file.
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

# Header-only. Populate without adding Cereal's own project, which builds tests.
FetchContent_Declare(cereal
  GIT_REPOSITORY https://github.com/USCiLab/cereal.git
  GIT_TAG v1.3.0
  GIT_SHALLOW TRUE
)

# configure.ac cloned https://github.com/postincrement/nativefiledialog.git, which
# is no longer available. mlabbe/nativefiledialog is the library documented in
# emulator/README.md and has the same source layout. The Ext dialog API used by
# gratze is provided by emulator/nfd_compat.
FetchContent_Declare(nativefiledialog
  GIT_REPOSITORY https://github.com/mlabbe/nativefiledialog.git
  GIT_TAG 67345b80ebb429ecc2aeda94c478b3bcc5f7888e
)

FetchContent_Declare(SDL_FontCache
  GIT_REPOSITORY https://github.com/grimfang4/SDL_FontCache.git
  GIT_TAG c37a4030e1a1ad22131acdf62e093001e67905d6
)

FetchContent_GetProperties(cereal)
if(NOT cereal_POPULATED)
  FetchContent_Populate(cereal)
endif()

FetchContent_GetProperties(nativefiledialog)
if(NOT nativefiledialog_POPULATED)
  FetchContent_Populate(nativefiledialog)
endif()

FetchContent_GetProperties(SDL_FontCache)
if(NOT sdl_fontcache_POPULATED)
  FetchContent_Populate(SDL_FontCache)
endif()

set(GRATZE_CEREAL_INCLUDE ${cereal_SOURCE_DIR}/include)
set(GRATZE_NFD_SOURCE ${nativefiledialog_SOURCE_DIR})
set(GRATZE_FONTCACHE_SOURCE ${sdl_fontcache_SOURCE_DIR})
set(GRATZE_GEN_INCLUDE ${CMAKE_BINARY_DIR}/include)

file(MAKE_DIRECTORY ${GRATZE_GEN_INCLUDE}/SDL_FontCache)
file(MAKE_DIRECTORY ${GRATZE_GEN_INCLUDE}/nativefiledialog)
file(COPY ${sdl_fontcache_SOURCE_DIR}/SDL_FontCache.h
     DESTINATION ${GRATZE_GEN_INCLUDE}/SDL_FontCache)
file(COPY ${nativefiledialog_SOURCE_DIR}/src/include/nfd.h
     DESTINATION ${GRATZE_GEN_INCLUDE}/nativefiledialog)

find_package(SDL2 QUIET)
if(SDL2_FOUND AND TARGET SDL2::SDL2)
  set(GRATZE_SDL2 SDL2::SDL2)
elseif(SDL2_FOUND AND TARGET SDL2::SDL2-static)
  set(GRATZE_SDL2 SDL2::SDL2-static)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
  set(GRATZE_SDL2 PkgConfig::SDL2)
endif()

find_package(SDL2_ttf QUIET)
if(SDL2_ttf_FOUND AND TARGET SDL2_ttf::SDL2_ttf)
  set(GRATZE_SDL2_TTF SDL2_ttf::SDL2_ttf)
elseif(TARGET SDL2::SDL2_ttf)
  set(GRATZE_SDL2_TTF SDL2::SDL2_ttf)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2_ttf REQUIRED IMPORTED_TARGET SDL2_ttf)
  set(GRATZE_SDL2_TTF PkgConfig::SDL2_ttf)
endif()

add_subdirectory(common)
add_subdirectory(disk)
add_subdirectory(starnet)
add_subdirectory(emulator)

add_executable(grzbin grzbin/grzbin.cc)
set_target_properties(grzbin PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/grzbin
)
target_link_libraries(grzbin PRIVATE grzcommon)

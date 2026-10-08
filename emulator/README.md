# gratze

Compiling On Linux or OSX
-------------------------
From the repository root:

    cmake -S . -B build
    cmake --build build

The `gratze` binary is `build/emulator/gratze`. CMake also builds `build/disk/grzdisk` and `build/grzbin/grzbin`.

Cereal, nativefiledialog, and SDL_FontCache are downloaded into the build tree. nativefiledialog is Michael Labbe's library. The postincrement fork named by the old configure script is no longer available, so `emulator/nfd_compat` supplies the dialog calls gratze uses. `z80asm` assembles `emulator/z80/cpm80/newbdos.asm` into the CP/M image while building.

Compiling with MingW
--------------------
Configure with a MinGW toolchain file, then build:

    cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=/path/to/mingw-toolchain.cmake
    cmake --build build

MingW
-----
  sudo apt-get install g++-mingw-w64-i686 cmake

Linux
-----
  sudo apt-get install cmake g++ pkg-config z80asm
  sudo apt-get install libsdl2-dev libsdl2-ttf-dev libfreetype6-dev
  sudo apt-get install libncurses-dev libgtk-3-dev

macOS
-----
  brew install cmake sdl2 sdl2_ttf z80asm
 
Various I/O Ports
-----------------
  http://www.trs-80.com/wordpress/zaps-patches-pokes-tips/ports-and-i-o-devices/
  http://interbutt.com/mess/super80/
  http://www.trailingedge.com/exidy/
  http://www.mjbauer.biz/DREAM6800.htm

Disk Formats
------------
  https://www.tim-mann.org/trs80/dskspec.html
  http://www.trs-80.com/wordpress/dsk-and-dmk-image-utilities/

Cassette formats
----------------
  https://github.com/Fortyseven/TRS-80-Model-II/blob/master/_Source%20Code/xtrs-4.9a/cassette.man
  http://www.topherlee.com/software/pcm-tut-wavformat.html

CPU Emulators
-------------
  Z80 : Marat Fayzullin
        http://fms.komkon.org/EMUL8/

  2650 :  Frank Pocnet
          http://www.tubedata.org/phunsy/index.html
Misc:
-----
  NFD (Native file dialogs):  Michael Labbe
                              https://github.com/mlabbe/nativefiledialog

  SDLFontCache: Jonathan Dearborn
                https://github.com/grimfang4/SDL_FontCache/blob/master/LICENSE

LICENSES
--------
  2650 emulator : No license :)

  libSDL: SDL 2.0 and newer are available under the zlib license 
                  (https://www.zlib.net/zlib_license.html)

  NFD : zlib license                
        (https://www.zlib.net/zlib_license.html)

  SDLFontCache: MIT license
                https://opensource.org/licenses/MIT      

  Z80 emulator :  Copyright (C) Marat Fayzullin 1994-2007
                  You are not allowed to distribute this software 
                  commercially. Please, notify me, if you make any 
                  changes to this file.

----
  - OSX support
  - lower level cassette emulation
  - Model III and Model IV support
  - state save and load
  - 40/80 video support
  - Model IV F1-F4 keys
  - Model III shift keys
  - Fix disk drive interface
  - Fix TRS-80 chars
  - DG640 flashing

Done
----  
  - CPU speed setting
  - cmake
  - linux support
  - 32/64 video mode
  - Refactor video support
  - Refactor disk drive code
  - Use fonts for video
  - use generic keyboard scanner
  - DG640 graphics
  - Better command line parsing

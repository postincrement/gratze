# gratze

Compiling On Linux or OSX
-------------------------
Use the following commands:
    aclocal
    autoconf
    ./configure
    make

Compiling on using MingW
------------------------
Select one of following target type:

    i686-mingw32          Using MingW32, compiling for Win32  
    i686-w64-mingw32      Using MingW64, compiling for Win32  
    x86_64-w64-mingw32    Using MingW64, compiling for Win64

Use the following commands:

    aclocal
    autoconf
    ./configure --host=TARGET
    make

MingW
-----
  sudo apt-get install g++-mingw-w64-i686
 
Various I/O Ports
-----------------
  http://www.trs-80.com/wordpress/zaps-patches-pokes-tips/ports-and-i-o-devices/

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
  - Refactor disk drive code
  - Refactor video support
  - Use fonts for video
  - lower level cassette emulation
  - Model III and Model IV support
  - state save and load
  - 32/64/40/80 support
  - Model IV F1-F4 keys
  - Model III shift keys

Done
----  
  - CPU speed setting
  - autoconf
  - linux support
  - Fix disk drive interface

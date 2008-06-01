#!/bin/sh
i686-mingw32-windres -i toxy_win.rc -o icon.o -O coff
i686-mingw32-gcc toxy_win.c icon.o -o toxy_win.exe -static -mwindows -llo -lpthread -lws2_32 -I/opt/local/static/include -L/opt/local/static/lib
i686-mingw32-strip toxy_win.exe

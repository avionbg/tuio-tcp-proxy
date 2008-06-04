#!/bin/sh
#WINDRES =
i686-mingw32-windres -i toxy_win.rc -o icon.o -O coff
#i686-mingw32-windres -i toxy_win.res -o icon.o -O coff
i686-mingw32-gcc toxy_win.c icon.o -o toxy_win.exe -static -mwindows -lshlwapi -lcomdlg32 -lcomctl32 -llo -lpthread -lws2_32 -I/opt/local/static/include -L/opt/local/static/lib
i686-mingw32-strip toxy_win.exe

::x86_64-w64-mingw32-windres version.rc -O coff -o version.res
::x86_64-w64-mingw32-gcc -O2 hotcorner.c version.res -o hotcorner.exe -Wl,-subsystem,windows

::version info
"C:\msys64\ucrt64\bin\windres.exe" version.rc -O coff -o .\build\version.res

::compilation
"C:\msys64\ucrt64\bin\gcc.exe" -O2 hotcorner.c .\build\version.res -o .\build\volumouse.exe -Wl,-subsystem,windows

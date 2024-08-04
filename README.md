# Felix
Atari Lynx emulator

## Building

Currently build only under windows, but it supposed to work in Wine, Parallels and other such environment.

Requirements:
- Visual studio 2022 
- Git
- CMake

### Prerequisites

```
git clone https://github.com/laoo/Felix.git
cd Felix
git submodule update --init --recursive
```

### IDE way

- open `Felix` folder with Visual Studio 2022
- in Solution Explorer set root "CMakeLists.txt" as a Startup Item
- build
- run

### command line way

```
md ..\Build
cd ..\Build
cmake ..\Felix
cmake --build . --config Release
Release\Felix.exe
```



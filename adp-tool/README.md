Building

Windows CMD
```
git clone https://github.com/microsoft/vcpkg
set VCPKG_ROOT=vcpkg
set PATH=%VCPKG_ROOT%;%PATH%
cmake --preset=default
cmake --build build
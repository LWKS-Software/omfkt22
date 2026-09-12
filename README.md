# omfkt22


## macOS

Do two builds, one for x86_64 and one for arm64
```
CFG=Release
ARCH2=x86_64
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=${CFG} -DCMAKE_OSX_ARCHITECTURES=${ARCH2}
cmake --build . --config ${CFG} -j4
```
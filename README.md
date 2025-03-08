## Universal image converter

![C++17](https://img.shields.io/badge/С++17-00599C)
![LibJPEG](https://img.shields.io/badge/LibJPEG-009B77)
![CMake](https://img.shields.io/badge/CMake-D1242F)

#### Description
The program converts images between different formats. It supports formats BMP, PPM, JPEG.

#### Build
```Bash
mkdir -p /build && cd /build && cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
```

#### Run
```Bash
imgconv <in_file> <out_file>
```

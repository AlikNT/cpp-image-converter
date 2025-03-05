## Universal image converter

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

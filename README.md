# WFLZW
WFLZW 1.0.0 is a C++ library for compressing and decompressing data. It is by its
design extremely light-weight, fast and memory-efficient (it can be configured to require as
little space as less than 1 kilobyte, all the way up to gigabytes). For this reason it may be
especially suitable for embedded systems with very restricted memory, but it's perfectly
usable in any platform.

The library consists of one single header file, with a relatively small amount of C++11
standard-compliant code. No myriads of source files, no configuration scripts required.
Its usage is very simple: Just `#include` the header file in any source code
where you want to use the library, and that's it.

Example benchmark (more benchmark results are included in the HTML documentation):

The file [rfc1812.txt](https://www.rfc-editor.org/rfc/rfc1812.txt), 415740 bytes in size:

| Dictionary size | Size of encoder (using list) | Size of encoder (using tree) | Size of decoder | Compressed data size | % of original | Compression time (using list) | Compression time (using tree) | Decompression time |
|:---------------:|:----------------------------:|:----------------------------:|:---------------:|:--------------------:|:-------------:|:-----------------------------:|:-----------------------------:|:------------------:|
| 4096 | 20 kB | 28 kB | 16 kB | 196722 | 47.3% | 6.8 ms | 5.8 ms | 3.4 ms |
| 8192 | 40 kB | 56 kB | 32 kB | 180965 | 43.5% | 7.4 ms | 5.9 ms | 3.0 ms |
| 16384 | 80 kB | 112 kB | 64 kB | 168806 | 40.6% | 8.2 ms | 5.6 ms | 2.9 ms |
| 32768 | 160 kB | 224 kB | 128 kB | 157587 | 37.9% | 9.5 ms | 5.7 ms | 2.7 ms |
| 65536 | 320 kB | 448 kB | 256 kB | 148570 | 35.7% | 11.2 ms | 5.7 ms | 2.5 ms |
| 131072 | 1152 kB | 1664 kB | 768 kB | 143882 | 34.6% | 14.7 ms | 6.2 ms | 2.9 ms |

Consult the included HTML documentation for usage details.

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

<table>
 <tr><th>Dictionary<br />size</th>
     <th>Size of encoder<br />(using list)</th>
     <th>Size of encoder<br />(using tree)</th>
     <th>Size of decoder<br /></th></tr>
     <th>Compressed<br />data size</th>
     <th>% of<br />original</th>
     <th>Compression time<br />(using list)</th>
     <th>Compression time<br />(using tree)</th>
     <th>Decompression<br />time</th></tr>
 <tr><td>4096</td><td>20 kB</td><td>28 kB</td><td>16 kB</td><td>196722</td><td>47.3%</td><td>6.8 ms</td><td>5.8 ms</td><td>3.4 ms</td></tr>
 <tr><td>8192</td><td>40 kB</td><td>56 kB</td><td>32 kB</td><td>180965</td><td>43.5%</td><td>7.4 ms</td><td>5.9 ms</td><td>3.0 ms</td></tr>
 <tr><td>16384</td><td>80 kB</td><td>112 kB</td><td>64 kB</td><td>168806</td><td>40.6%</td><td>8.2 ms</td><td>5.6 ms</td><td>2.9 ms</td></tr>
 <tr><td>32768</td><td>160 kB</td><td>224 kB</td><td>128 kB</td><td>157587</td><td>37.9%</td><td>9.5 ms</td><td>5.7 ms</td><td>2.7 ms</td></tr>
 <tr><td>65536</td><td>320 kB</td><td>448 kB</td><td>256 kB</td><td>148570</td><td>35.7%</td><td>11.2 ms</td><td>5.7 ms</td><td>2.5 ms</td></tr>
 <tr><td>131072</td><td>1152 kB</td><td>1664 kB</td><td>768 kB</td><td>143882</td><td>34.6%</td><td>14.7 ms</td><td>6.2 ms</td><td>2.9 ms</td></tr>
</table>

Consult the included HTML documentation for usage details.

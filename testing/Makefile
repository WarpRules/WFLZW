CFLAGS=-Wall -Wextra -pedantic -O3 -march=native

test_wflzw: test.cc ../WFLZW.hh
	g++ $(CFLAGS) test.cc -o $@
	strip $@.exe

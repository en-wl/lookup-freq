
all: import normalize

common.o: common.cpp util.hpp schema.hpp
	/opt/gcc/bin/g++-4.9 -Wl,-rpath,/opt/gcc/lib64 -std=gnu++11 -Wall -Wno-narrowing  -O2 common.cpp -c -o common.o

import: import.cpp common.o util.hpp schema.hpp
	/opt/gcc/bin/g++-4.9 -Wl,-rpath,/opt/gcc/lib64 -std=gnu++11 -Wall -Wno-narrowing  -O2 common.o import.cpp -o import

normalize: normalize.cpp
	/opt/gcc/bin/g++-4.9 -Wl,-rpath,/opt/gcc/lib64 -std=gnu++11 -Wall -Wno-narrowing  -O2 normalize.cpp -o normalize

dump: dump.cpp
	/opt/gcc/bin/g++-4.9 -Wl,-rpath,/opt/gcc/lib64 -std=gnu++11 -Wall -Wno-narrowing  -O2 dump.cpp -o dump

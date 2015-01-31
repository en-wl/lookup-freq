
CMPH_LIB := /opt/cmph/lib
CMPH_INCL := /opt/cmph/include

lookup: lookup.cpp
	/opt/gcc/bin/g++-4.9 -std=gnu++14 -Wall -Wno-narrowing -O2 -Wno-missing-braces -I${CMPH_INCL} lookup.cpp -Wl,-rpath,/opt/gcc/lib64 -Wl,-rpath,${CMPH_LIB} -L${CMPH_LIB} -lcmph -laspell -o lookup

.PHONY: dist
dist:
	mkdir ngrams-lookup
	cp Makefile lookup.cpp *.hpp ngrams-lookup/
	tar cfz ngrams-lookup.tgz ngrams-lookup

	cp ngrams.* SpellerStats*.dat en-lower.rws myen.dat myen_phonet.dat ngrams-lookup/
	tar cfz ngrams-lookup-w-data.tgz ngrams-lookup



all: cz2_debug

version:
	./update_version

cz2_debug: version
	gcc -x c++ -lstdc++ -std=c++11 -o cz2_debug cz2_debug.cpp RingBuffer.cpp Util.cpp ComfortZoneII.cpp

readme:
	mdv README.md

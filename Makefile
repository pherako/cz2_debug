all:
	gcc -x c++ -lstdc++ -std=c++11 -o cz2_debug parse_log.cpp RingBuffer.cpp Util.cpp ComfortZoneII.cpp

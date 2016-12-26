dm:decafmind.tab.cpp decafmind.cpp
	printf "\033[01;47;31m                                                                                                         \033[0m\n"
	g++ -o dm decafmind.tab.cpp decafmind.cpp -Wno-return-type -Wno-switch -Wno-unused-value -Wno-deprecated-register -std=c++11
	make clean

decafmind.tab.cpp:decafmind.y
	bison -d decafmind.y
	mv decafmind.tab.c decafmind.tab.cpp
	mv decafmind.tab.h decafmind.tab.hpp

decafmind.cpp:decafmind.l
	flex -o decafmind.cpp decafmind.l

clean:
	rm -f decafmind.tab.cpp
	rm -f decafmind.tab.hpp
	rm -f decafmind.cpp

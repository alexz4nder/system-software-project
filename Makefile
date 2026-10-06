asembler_src := $(wildcard src/asembler/*.cpp) 
emulator_src := $(wildcard src/emulator/*.cpp) 
linker_src := $(wildcard src/linker/*.cpp)
common_src := $(wildcard src/common/*.cpp)


asembler:
	g++ -g -o build/asembler $(asembler_src) $(common_src) 

emulator:
	g++ -std=c++17 -pthread -g -o build/emulator $(emulator_src) $(common_src) 
linker:
	g++ -g -o build/linker $(linker_src) $(common_src)  -Wno-write-strings


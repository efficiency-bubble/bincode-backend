COMPOPT := -std=c++26 -flto -fuse-linker-plugin -Wall -Wextra -Wpedantic -m64 -O3 -I"src" $(cppinclude)
FINALOPT := $(COMPOPT) -s -static
HEADERS := $(wildcard src/bbe/*.hpp)
SOURCES := $(wildcard src/bbe/*.cpp)
OBJECTS := $(patsubst %.cpp,%.o,$(SOURCES))
ifeq ($(OS),"Windows_NT")
TARGET_SUFFIX := .exe
else
TARGET_SUFFIX :=
endif
test$(TARGET_SUFFIX): test.cpp bbe.a
	g++ $< bbe.a -o $@ $(FINALOPT)
backend$(TARGET_SUFFIX): backend.cpp bbe.a
	g++ $< bbe.a -o $@ $(FINALOPT)
bbe.a: $(OBJECTS)
	ar rcs $@ $(OBJECTS)
%.o: %.cpp $(HEADERS)
	g++ -c $< -o $@ $(COMPOPT)

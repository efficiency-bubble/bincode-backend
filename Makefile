COMPOPT := -std=c++26 -flto=7 -fuse-linker-plugin -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wno-maybe-uninitialized -m64 -I"src/include" -I$(cppinclude)
RELEASEOPT := -O3 -s -DNDEBUG
DEBUGOPT := -O0 -g
FINALOPT := $(COMPOPT) -L$(cpplibs) -ltbb
LIBNAME := bbe
HEADERS := $(wildcard src/include/$(LIBNAME)/*.hpp) $(wildcard src/include/$(LIBNAME)/*/*.hpp)
SOURCES := $(wildcard src/implementation/*.cpp) $(wildcard src/implementation/*/*.cpp)
OBJECTS := $(patsubst src/implementation/%.cpp,obj/%.o,$(SOURCES))
DEBUG_OBJECTS := $(patsubst src/implementation/%.cpp,obj/%_debug.o,$(SOURCES))
all: lib/$(LIBNAME).a lib/$(LIBNAME)_debug.a bin/bcc
	
bin/%: src/%.cpp lib/$(LIBNAME).a
	g++ $< lib/$(LIBNAME).a -o $@ $(RELEASEOPT) $(FINALOPT)
bin/%_debug: src/%.cpp lib/$(LIBNAME)_debug.a
	g++ $< lib/$(LIBNAME)_debug.a -o $@ $(DEBUGOPT) $(FINALOPT)
lib/$(LIBNAME).a: $(OBJECTS)
	ar rcs $@ $(OBJECTS)
lib/$(LIBNAME)_debug.a: $(DEBUG_OBJECTS)
	ar rcs $@ $(DEBUG_OBJECTS)
obj/%.o: src/implementation/%.cpp $(HEADERS)
	g++ -c $< -o $@ $(COMPOPT) $(RELEASEOPT)
obj/%_debug.o: src/implementation/%.cpp $(HEADERS)
	g++ -c $< -o $@ $(COMPOPT) $(DEBUGOPT)
setup:
	mkdir lib
	mkdir bin
	mkdir obj
	mkdir obj/formats
	mkdir obj/inter
	mkdir obj/targets
clean:
	find obj -type f -delete
	find bin -type f -delete
	find lib -type f -delete
.PHONY: setup all clean
	
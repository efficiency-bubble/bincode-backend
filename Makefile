COMPOPT := -std=c++26 -flto -fuse-linker-plugin -Wall -Wextra -Wpedantic -Wconversion -m64 -O0 -g -I"src/include" $(cppinclude)
FINALOPT := $(COMPOPT) -static
LIBNAME := bbe
HEADERS := $(wildcard src/include/$(LIBNAME)/*.hpp) $(wildcard src/include/$(LIBNAME)/*/*.hpp)
SOURCES := $(wildcard src/implementation/*.cpp) $(wildcard src/implementation/*/*.cpp)
OBJECTS := $(patsubst src/implementation/%.cpp,obj/%.o,$(SOURCES))
bin/%: src/%.cpp lib/$(LIBNAME).a
	g++ $< lib/bbe.a -o $@ $(FINALOPT)
lib/$(LIBNAME).a: $(OBJECTS)
	ar rcs $@ $(OBJECTS)
obj/%.o: src/implementation/%.cpp $(HEADERS)
	g++ -c $< -o $@ $(COMPOPT)

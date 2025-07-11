COMPOPT := -std=c++26 -flto -fuse-linker-plugin -Wall -Wextra -Wpedantic -m64 -O0 -g -I"src/include" $(cppinclude)
FINALOPT := $(COMPOPT) -static
LIBNAME := bbe
HEADERS := $(wildcard src/include/$(LIBNAME)/*.hpp) $(wildcard src/include/$(LIBNAME)/*/*.hpp)
SOURCES := $(wildcard src/implementation/*.cpp)
OBJECTS := $(patsubst src/implementation/%.cpp,obj/%.o,$(SOURCES))
ifeq ($(OS),"Windows_NT")
TARGET_SUFFIX := .exe
else
TARGET_SUFFIX :=
endif
bin/%$(TARGET_SUFFIX): src/%.cpp lib/$(LIBNAME).a
	g++ $< lib/bbe.a -o $@ $(FINALOPT)
lib/$(LIBNAME).a: $(OBJECTS)
	ar rcs $@ $(OBJECTS)
obj/%.o: src/implementation/%.cpp $(HEADERS)
	g++ -c $< -o $@ $(COMPOPT)

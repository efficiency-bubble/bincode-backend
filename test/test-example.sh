../bin/test \
&& objdump -d test.o\
&& g++ test-main.cpp test.o -o test-main -std=c++26 -Wall -Wextra -Wpedantic -m64 -O3 \
&& ./test-main

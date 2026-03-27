#include<cinttypes>
#include<cstdint>
#include<cstdio>
extern "C" {
    std::uint32_t example(std::uint32_t i);
}

constexpr static std::uint32_t MAX_ERRORS = 15;
int main(){
    std::uint32_t fail = 0;
    std::uint32_t all = 0;
    std::uint32_t a = 0;
    std::uint32_t b = 1;
    for(std::uint32_t i=1;i<30;++i){
        ++all;
        std::uint32_t result = example(i);
        std::uint32_t c = a + b;
        a = b;
        b = c;
        if(result!=a){
            if(fail < MAX_ERRORS){
                printf("FAIL: Code erroneously says that fib %" PRIu32 " is %" PRIu32 " (should be %" PRIu32 ") \n",i,result,a);
            }
            ++fail;
        }
    }
    if(fail > MAX_ERRORS){
        printf("... %" PRIu32 " more errors(s) ...\n",fail-MAX_ERRORS);
    }
    printf("%" PRIu32 "/%" PRIu32 " tests passed\n",all-fail,all);
    return 0;
}

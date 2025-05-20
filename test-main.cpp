#include<cinttypes>
#include<cstdint>
#include<cstdio>
extern "C"{
    std::uint32_t plus_n(std::uint32_t x);
    std::uint32_t n = 7;
}
constexpr static std::uint32_t MAX_ERRORS = 15;
int main(){
    std::uint32_t fail = 0;
    std::uint32_t all = 0;
    for(n=0;n<256;++n){
        for(std::uint32_t i=0;i<256;++i){
            ++all;
            std::uint32_t result = plus_n(i);
            std::uint32_t expected = i+n;
            if(result!=expected){
                if(fail < MAX_ERRORS){
                    printf("FAIL: Code erroneously says %" PRIu32 " + %" PRIu32 " is %" PRIu32 " (should be %" PRIu32 ") \n",n,i,result,expected);
                }
                ++fail;
            }
        }
    }
    if(fail > MAX_ERRORS){
        printf("... %" PRIu32 " more errors(s) ...\n",fail-MAX_ERRORS);
    }
    printf("%" PRIu32 "/%" PRIu32 " tests passed\n",all-fail,all);
    return 0;
}

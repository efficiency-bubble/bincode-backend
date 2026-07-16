#include<cinttypes>
#include<cstdint>
#include<cstdio>
extern "C" {
    std::uint32_t example(std::uint32_t i,std::uint32_t j);
    std::uint32_t multiply_adjust(std::uint32_t i,std::uint32_t j){
        return i * j + i;
    }
}
std::uint32_t correct_behavior(std::uint32_t i,std::uint32_t j){
    if(i <= 2){
        return multiply_adjust(j,j) + 1;
    }else{
        return correct_behavior(i-1,j) + correct_behavior(i-2,j);
    }
}

constexpr static std::uint32_t MAX_ERRORS = 15;
int main(){
    std::uint32_t fail = 0;
    std::uint32_t all = 0;
    for(std::uint32_t i=1;i<30;++i){
        for(std::uint32_t begin=5;begin<33;begin += 3){
            std::uint32_t result = example(i,begin);
            std::uint32_t correct = correct_behavior(i,begin);
            ++all;
            if(result != correct){
                if(fail < MAX_ERRORS){
                    printf("FAIL: Code erroneously says that test %" PRIu32 " / %" PRIu32 " is %" PRIu32 " (should be %" PRIu32 ") \n",i,begin,result,correct);
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

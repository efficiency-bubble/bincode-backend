#include<iostream>
#include<cstdint>
extern "C"{
    std::int32_t example();
    std::int32_t value = 7;
}
int main(){
    for(value=0;value<20;++value){
        std::cout << value << " - 6 is: " << example() << '\n';
    }
    std::cout.flush();
    return 0;
}

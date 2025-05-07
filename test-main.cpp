#include<iostream>
extern "C" int example();
int main(){
    std::cout << "Example value is: " << example() << std::endl;
    return 0;
}

#pragma once
#include"../commons.hpp"
#include<cppp/bfile.hpp>
namespace bbe::formats{
    consteval inline std::byte operator ""_b(unsigned long long x){
        return static_cast<std::byte>(x);
    }
    consteval inline std::byte operator ""_b(char x){
        return static_cast<std::byte>(x);
    }
}

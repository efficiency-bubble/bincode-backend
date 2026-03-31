#pragma once
#include"commons.hpp"
#include<vector>
namespace bbe::impl{
    class TypeLayout{
        uint64_t _size;
        uint64_t _align;
        public:
            TypeLayout(uint64_t sz,uint64_t al) : _size(sz), _align(al){}
            uint64_t size() const{
                return _size;
            }
            uint64_t alignment() const{
                return _align;
            }
    };
    class Type{
        std::uint32_t tname;
        std::vector<Type> targs;
        public:
            constexpr static std::uint32_t T_VOID = 0;
            constexpr static std::uint32_t T_UINT32 = 1;
            Type(std::uint32_t tn,std::vector<Type>&& targs) : tname(tn), targs(std::move(targs)){}
            Type(std::uint32_t tn) : tname(tn){}
    };
}
namespace bbe{
    BBE_EXPORT TypeLayout;
    BBE_EXPORT Type;
}

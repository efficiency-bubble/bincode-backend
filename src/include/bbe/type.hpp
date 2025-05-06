#pragma once
#include"commons.hpp"
namespace bbe::impl{
    class Type{
        uint64_t _size;
        uint64_t _align;
        public:
            Type(uint64_t sz,uint64_t al) : _size(sz), _align(al){}
            uint64_t size() const{
                return _size;
            }
            uint64_t alignment() const{
                return _align;
            }
    };
}
namespace bbe{
    BBE_EXPORT Type;
}

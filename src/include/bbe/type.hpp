#pragma once
#include"commons.hpp"
namespace bbe::impl{
    class Type{
        str _name;
        uint64_t _size;
        uint64_t _align;
        public:
            Type(str n,uint64_t sz,uint64_t al) : _name(std::move(n)), _size(sz), _align(al){}
            const str& name() const{
                return _name;
            }
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

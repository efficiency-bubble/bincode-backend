#pragma once
#include"../targets/rtl.hpp"
#include"entity_pool.hpp"
#include"value.hpp"
namespace bbe::inter::rtl::impl{
    using namespace bbe::impl;
    class CompiledFunctionPool : public inter::impl::CompiledFunctionPool<targets::rtl::Function>{
        public:
            using inter::impl::CompiledFunctionPool<targets::rtl::Function>::CompiledFunctionPool;
            Value call(ProjectEntitiesPool::index_type,const Value&) const;
    };
}
namespace bbe::inter::rtl{
    BBE_EXPORT CompiledFunctionPool;
}

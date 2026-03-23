#pragma once
#include"../targets/dfg.hpp"
#include<cppp/object-view.hpp>
#include"entity_pool.hpp"
#include<unordered_map>
#include"value.hpp"
namespace bbe::inter::dfg::impl{
    using namespace bbe::impl;
    class CompiledFunctionPool : public inter::impl::CompiledFunctionPool<targets::dfg::DataFlowGraph>{
        public:
            using inter::impl::CompiledFunctionPool<targets::dfg::DataFlowGraph>::CompiledFunctionPool;
            Value call(ProjectEntitiesPool::index_type,cppp::view<const Value>) const;
    };
}
namespace bbe::inter::dfg{
    BBE_EXPORT CompiledFunctionPool;
}

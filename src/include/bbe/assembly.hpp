#pragma once
#include<unordered_map>
#include<vector>
#include<queue>
#include<cppp/bytearray.hpp>
#include<cppp/virtual.hpp>
#include"function.hpp"
#include"commons.hpp"
namespace bbe::impl{
    using cppp::bytes;
    struct RelocationList{
        std::vector<std::size_t> offsets;
    };
    class Text{
        bytes instr;
        std::unordered_map<std::uint64_t,RelocationList> refs;
        public:
            bytes& text(){
                return instr;
            }
            const bytes& text() const{
                return instr;
            }
    };
    class Compiler : public cppp::virtual_class{
        public:
            virtual void compile(const Function&,Text&) const = 0;
    };
    namespace targets{
        class Defaultx64 : public Compiler{
            public:
                void compile(const Function&,Text&) const override;
        };
    }
}
namespace bbe{
    BBE_EXPORT bytes;
    BBE_EXPORT RelocationList;
    BBE_EXPORT Text;
    BBE_EXPORT Compiler;
    namespace targets = impl::targets;
}

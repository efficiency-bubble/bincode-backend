#pragma once
#include<unordered_map>
#include<vector>
#include<queue>
#include<cppp/bytearray.hpp>
#include<cppp/virtual.hpp>
#include<cppp/strmap.hpp>
#include<cppp/string.hpp>
#include"function.hpp"
#include"commons.hpp"
namespace bbe::impl{
    using cppp::bytes;
    struct Symbol{
        std::uint64_t offset;
        std::uint64_t size;
    };
    class Text{
        bytes instr;
        cppp::strmap<Symbol> function_exports;
        public:
            bytes& text(){
                return instr;
            }
            const bytes& text() const{
                return instr;
            }
            const cppp::strmap<Symbol>& exports() const{
                return function_exports;
            }
            template<typename F>
            void add_function(cppp::str&& s,const F& f){
                std::uint64_t begin{static_cast<std::uint64_t>(instr.size())};
                f(*this);
                std::uint64_t end{static_cast<std::uint64_t>(instr.size())};
                function_exports.try_emplace(std::move(s),begin,end-begin);
            }
    };
    class Compiler : public cppp::virtual_class{
        public:
            virtual void compile(const Function&,Text&) const = 0;
    };
}
namespace bbe{
    BBE_EXPORT bytes;
    BBE_EXPORT Text;
    BBE_EXPORT Compiler;
}

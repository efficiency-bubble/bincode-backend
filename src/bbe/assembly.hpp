#pragma once
#include"commons.hpp"
#include"ast.hpp"
#include<cppp/bytearray.hpp>
#include<unordered_map>
#include<vector>
#include<queue>
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
    class Allocator{
        std::uint64_t max;
        std::uint64_t length;
        std::priority_queue<std::uint64_t> freelist;
        public:
            Allocator() : max(0), length(0){}
            std::uint64_t size() const{
                return length;
            }
            std::uint64_t max_size() const{
                return max;
            }
            std::uint64_t push(){
                if(freelist.empty()){
                    if(++length > max) max = length;
                    return length - 1;
                }else{
                    std::size_t ind = freelist.top();
                    freelist.pop();
                    return ind;
                }
            }
            void pop(std::uint64_t ind){
                freelist.push(ind);
                std::uint64_t ls;
                while(!freelist.empty()&&freelist.top()==(ls = length-1)){
                    freelist.pop();
                    length = ls;
                }
            }
    };
    class FunctionCompilationContext{
        Text* _text;
        Allocator _stack;
        public:
            FunctionCompilationContext(Text& t) : _text(&t){}
            Text& text(){
                return *_text;
            }
            const Text& text() const{
                return *_text;
            }
            Allocator& stack(){
                return _stack;
            }
            const Allocator& stack() const{
                return _stack;
            }
    };
}
namespace bbe{
    BBE_EXPORT bytes;
    BBE_EXPORT RelocationList;
    BBE_EXPORT Text;
    BBE_EXPORT FunctionCompilationContext;
}

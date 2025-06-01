#pragma once
#include<cppp/preprocessor.hpp>
#include<cppp/freelist.hpp>
#include<cppp/string.hpp> // printing enum names
#include"commons.hpp"
#include"../function.hpp"
#include<vector>
namespace bbe::targets::ssa::impl{
    using namespace std::literals;
    /*
    Example C++:
    
    void loop(unsigned int a){
        while(a){
            a = run(a);
        }
    }
    
    Example SSA in C syntax:
    
    static unsigned int __jargbuf[1];
    void loop(const unsigned int __var0){
        __jargbuf[0] = __var0;
        __label0:
        const unsigned int __var1 = __jargbuf[0];
        const unsigned int __var2 = run(__var1);
        const unsigned int __var3 = 0;
        const unsigned int __var4 = __var3 < __var2;
        if(__var4){
            __jargbuf[0] = __var2;
            goto __label0;
        }
    }  
    */
    #define BBE_DEBUG_STRINGIFY_ENUMERATOR(x) u8 ## #x ## sv
    #define BBE_DEBUG_NAMED_ENUM(e,...) enum class e { __VA_ARGS__ }; constexpr cppp::sv stringify_enum(e v){ constexpr static cppp::sv enum_strings[]{ CPPP_FOR_EACH(BBE_DEBUG_STRINGIFY_ENUMERATOR,__VA_ARGS__) }; return enum_strings[static_cast<std::size_t>(v)]; }
    BBE_DEBUG_NAMED_ENUM(Operation,IMMB,IMM32,IMM64,PUTV,LOADV,JMP,MOV,RET,ADD,SUB,CMPL,LDAR);
    struct Instruction{
        Operation opcode;
        std::uint32_t dst;
        std::uint32_t srcp;
        std::uint32_t srcq;
        cppp::str debug() const{
            cppp::str string{cppp::tou8(std::to_string(dst))};
            string.append(u8": "sv);
            string.append(stringify_enum(opcode));
            string.push_back(u8' ');
            string.append(cppp::tou8(std::to_string(srcp)));
            string.push_back(u8' ');
            string.append(cppp::tou8(std::to_string(srcq)));
            return string;
        }
    };
    class ProcedureIC{
        std::uint32_t next_value = 0;
        std::vector<Instruction> _instructions;
        std::vector<std::uint32_t> name_values;
        struct Label{
            std::uint32_t begin;
            std::vector<std::uint32_t> param_names;
        };
        std::vector<Label> labels;
        std::uint32_t new_value_for_name(std::uint32_t name){
            std::uint32_t vid = next_value++;
            if(name==new_name()){
                name_values.emplace_back(vid);
            }else{
                name_values[name] = vid;
            }
            return vid;
        }
        public:
            std::uint32_t new_label_here();
            std::uint32_t new_name() const{
                return static_cast<std::uint32_t>(name_values.size());
            }
            std::uint32_t value_of(std::uint32_t name){
                return name_values[name];
            }
            void goto_label(std::uint32_t label){
                for(std::uint32_t param : labels[label].param_names){
                    _instructions.emplace_back(Operation::PUTV,param,name_values[param]);
                }
                _instructions.emplace_back(Operation::JMP,label);
            }
            void statement(Operation op,std::uint32_t lhsv=0,std::uint32_t rhsv=0){
                _instructions.emplace_back(op,0,lhsv,rhsv);
            }
            void operation(Operation op,std::uint32_t name,std::uint32_t lhsv=0,std::uint32_t rhsv=0){
                _instructions.emplace_back(op,new_value_for_name(name),lhsv,rhsv);
            }
            void immb(std::uint32_t name,bool val){
                operation(Operation::IMMB,name,val);
            }
            void imm32(std::uint32_t name,std::uint32_t val){
                operation(Operation::IMM32,name,val);
            }
            void imm64(std::uint32_t name,std::uint64_t val){
                operation(Operation::IMM64,name,static_cast<std::uint32_t>(val),static_cast<std::uint32_t>(val>>32uz));
            }
            const std::vector<Instruction>& instructions() const{
                return _instructions;
            }
            void compile(const Function&);
    };
}
namespace bbe::targets::ssa{
    BBE_EXPORT Instruction;
    BBE_EXPORT ProcedureIC;
}

#pragma once
#include<cppp/preprocessor.hpp>
#include<cppp/freelist.hpp>
#include<cppp/string.hpp> // printing enum names
#include"commons.hpp"
#include"../function.hpp"
#include<unordered_map>
#include<limits>
#include<vector>
#include<deque>
namespace bbe::targets::ssa::impl{
    using namespace std::literals;
    #define BBE_DEBUG_STRINGIFY_ENUMERATOR(x) u8 ## #x ## sv
    #define BBE_DEBUG_NAMED_ENUM(e,...) enum class e { __VA_ARGS__ }; constexpr cppp::sv stringify_enum(e v){ constexpr static cppp::sv enum_strings[]{ CPPP_FOR_EACH(BBE_DEBUG_STRINGIFY_ENUMERATOR,__VA_ARGS__) }; return enum_strings[static_cast<std::size_t>(v)]; }
    BBE_DEBUG_NAMED_ENUM(Operation,IMMB,IMM32,IMM64,PUTV,CALL,JMP,BRC,MOV,RET,ADD,SUB,CMPL,LDAR);
    struct Instruction{
        Operation opcode;
        std::uint32_t dst;
        std::vector<std::uint32_t> src;
        cppp::str debug() const;
    };
    class BasicBlock{
        std::uint32_t next_value = 0;
        std::uint32_t retval;
        std::vector<Instruction> _instructions;
        std::unordered_map<std::uint32_t,std::uint32_t> name_values;
        std::unordered_map<std::uint32_t,std::uint32_t> _imports;
        std::uint32_t new_value_for_name(std::uint32_t name);
        public:
            constexpr static std::uint32_t NNAME = std::numeric_limits<std::uint32_t>::max();
            std::uint32_t value_of(std::uint32_t name){
                if(name==NNAME) return name;
                if(!name_values.contains(name)){
                    _imports.try_emplace(name,new_value_for_name(name));
                }
                return name_values.at(name);
            }
            void instruction(const Instruction& ins){
                _instructions.emplace_back(ins);
            }
            void retb(std::uint32_t val){
                retval = val;
            }
            void retf(std::uint32_t val){
                _instructions.emplace_back(Operation::RET,val);
            }
            void operation(Operation op,std::uint32_t name,std::vector<std::uint32_t>&& argv={}){
                _instructions.emplace_back(op,new_value_for_name(name),std::move(argv));
            }
            void immb(std::uint32_t name,bool val){
                operation(Operation::IMMB,name,{val});
            }
            void imm32(std::uint32_t name,std::uint32_t val){
                operation(Operation::IMM32,name,{val});
            }
            void imm64(std::uint32_t name,std::uint64_t val){
                operation(Operation::IMM64,name,{static_cast<std::uint32_t>(val),static_cast<std::uint32_t>(val>>32uz)});
            }
            const std::vector<Instruction>& instructions() const{
                return _instructions;
            }
            const std::unordered_map<std::uint32_t,std::uint32_t>& imports() const{
                return _imports;
            }
            std::uint32_t return_value() const{
                return retval;
            }
    };
    class ProcedureIC{
        std::deque<BasicBlock> _blocks; // no iterator invalidation
        public:
            std::uint32_t new_block(){
                std::uint32_t bid = _blocks.size();
                _blocks.emplace_back();
                return bid;
            }
            std::deque<BasicBlock>& blocks(){
                return _blocks;
            }
            const std::deque<BasicBlock>& blocks() const{
                return _blocks;
            }
            void compile(const Function&);
    };
    BasicBlock dce(const BasicBlock& prog);
}
namespace bbe::targets::ssa{
    BBE_EXPORT Instruction;
    BBE_EXPORT BasicBlock;
    BBE_EXPORT ProcedureIC;
    BBE_EXPORT dce;
}

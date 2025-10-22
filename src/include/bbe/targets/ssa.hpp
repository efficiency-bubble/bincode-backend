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
    BBE_DEBUG_NAMED_ENUM(Operation,IMMB,IMM32,IMM64,PUTV,PACK,CALL,CMAG,RET,LDAR,LDS,
        MOV,JMP,JCC // used for LJF
    );
    struct Instruction{
        Operation opcode;
        std::uint32_t dst;
        std::vector<std::uint32_t> src;
        cppp::str debug() const;
    };
    struct ValueAllocation{
        std::uint32_t next_value = 0;
    };
    class BasicBlock{
        ValueAllocation* va;
        std::uint32_t _retcond = NCOND;
        std::uint32_t _ret = NRET;
        std::uint32_t _ret2;
        std::vector<Instruction> _instructions;
        std::unordered_map<std::uint32_t,std::uint32_t> name_values;
        std::unordered_map<std::uint32_t,std::uint32_t> _imports;
        std::uint32_t new_value_for_name(std::uint32_t name);
        public:
            BasicBlock(ValueAllocation& va) : va(&va){}
            constexpr static std::uint32_t NCOND = std::numeric_limits<std::uint32_t>::max();
            constexpr static std::uint32_t NRET = std::numeric_limits<std::uint32_t>::max();
            constexpr static std::uint32_t NNAME = std::numeric_limits<std::uint32_t>::max();
            void bind_name(std::uint32_t name,std::uint32_t value){
                name_values.insert_or_assign(name,value);
            }
            std::uint32_t value_of(std::uint32_t name){
                if(name==NNAME) return name;
                if(!name_values.contains(name)){
                    _imports.try_emplace(name,new_value_for_name(name));
                }
                return name_values.at(name);
            }
            template<typename ...A>
            void instruction(A&& ...a){
                _instructions.emplace_back(std::forward<A>(a)...);
            }
            void r_always(std::uint32_t to){
                _retcond = NCOND;
                _ret = to;
            }
            std::uint32_t ret() const{
                return _ret;
            }
            std::uint32_t ret2() const{
                return _ret2;
            }
            void r_branch(std::uint32_t cond,std::uint32_t tru,std::uint32_t fals){
                _retcond = cond;
                _ret = tru;
                _ret2 = fals;
            }
            std::uint32_t retcond() const{
                return _retcond;
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
            const std::unordered_map<std::uint32_t,std::uint32_t>& nametable() const{
                return name_values;
            }
    };
    class ProcedureIC{
        ValueAllocation alloc;
        std::deque<BasicBlock> _blocks; // no iterator invalidation
        public:
            ProcedureIC(const Function&);
            constexpr static std::uint32_t NBLOCK = std::numeric_limits<std::uint32_t>::max();
            std::uint32_t new_block(){
                std::uint32_t bid = static_cast<std::uint32_t>(_blocks.size());
                _blocks.emplace_back(alloc);
                return bid;
            }
            std::deque<BasicBlock>& blocks(){
                return _blocks;
            }
            const std::deque<BasicBlock>& blocks() const{
                return _blocks;
            }
    };
}
namespace bbe::targets::ssa{
    BBE_EXPORT Operation;
    BBE_EXPORT Instruction;
    BBE_EXPORT BasicBlock;
    BBE_EXPORT ProcedureIC;
}

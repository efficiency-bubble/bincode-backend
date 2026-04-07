#pragma once
#include<cppp/bytearray.hpp>
#include<cppp/string.hpp> // symbol names
#include"../type.hpp"
#include"dfg.hpp"
#include"../project_entity_pool.hpp"
namespace bbe::targets::x86::impl{
    struct FunctionRelocation{
        std::uint32_t offset;
        ProjectEntitiesPool::index_type fni;
        // size of the instruction being relocated; since IP-relative 
        std::uint32_t isize;
    };
    class Function{
        cppp::bytes b;
        std::vector<FunctionRelocation> rels;
        public:
            Function(const dfg::Function& f,const TypeDatabase& tdb);
            void add_relocation(FunctionRelocation fr){
                rels.emplace_back(fr);
            }
            const std::vector<FunctionRelocation>& relocations() const{
                return rels;
            }
            cppp::bytes& instructions(){
                return b;
            }
            const cppp::bytes& instructions() const{
                return b;
            }
    };
    enum class SymbolType{
        FUNCTION,VARIABLE
    };
    class SymbolInfo{
        cppp::str _name;
        std::uint32_t _index;
        SymbolType _type;
        bool import;
        public:
            SymbolInfo(cppp::str n,SymbolType t,std::uint32_t i,bool import) : _name(std::move(n)), _index(i), _type(t), import(import){}
            const cppp::str& name() const{
                return _name;
            }
            SymbolType type() const{
                return _type;
            }
            std::uint32_t index() const{
                return _index;
            }
            bool imported() const{
                return import;
            }
    };
    class Program{
        std::vector<Function> fnv;
        std::vector<SymbolInfo> symtab;
        public:
            void add_function(Function&& fn){
                fnv.emplace_back(std::move(fn));
            }
            void export_function(cppp::str n,Function&& fn){
                symtab.emplace_back(std::move(n),SymbolType::FUNCTION,fnv.size(),false);
                add_function(std::move(fn));
            }
            const std::vector<SymbolInfo>& symbols() const{
                return symtab;
            }
            const std::vector<Function>& functions() const{
                return fnv;
            }
    };
}
namespace bbe::targets::x86{
    BBE_EXPORT FunctionRelocation;
    BBE_EXPORT Function;
    BBE_EXPORT SymbolType;
    BBE_EXPORT SymbolInfo;
    BBE_EXPORT Program;
}

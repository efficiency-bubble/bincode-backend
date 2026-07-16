#pragma once
#include<cppp/bytearray.hpp>
#include<cppp/string.hpp> // symbol names
#include"../type.hpp"
#include"dfg.hpp"
#include"../project_entity_pool.hpp"
namespace bbe::targets::x86::impl{
    struct FunctionRelocation{
        std::uint32_t offset;
        func_id fni;
        // size of the instruction being relocated; since IP-relative addressing is relative to the next instruction
        std::uint32_t isize;
    };
    class Function{
        cppp::sv _cname;
        cppp::bytes b;
        std::vector<FunctionRelocation> rels;
        public:
            Function(cppp::sv cn,const dfg::Function& f,const TypeDatabase& tdb);
            // stub constructor
            Function(cppp::sv cn) : _cname(cn){}
            cppp::sv cname() const{
                return _cname;
            }
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
        std::unordered_map<func_id,std::uint32_t> function_order;
        std::vector<SymbolInfo> symtab;
        public:
            void add_function(func_id fid,Function&& fn){
                function_order.try_emplace(fid,fnv.size());
                fnv.emplace_back(std::move(fn));
            }
            std::uint32_t get_index(func_id fid) const{
                return function_order.at(fid);
            }
            void import_function(func_id fid,cppp::sv cname){
                cppp::sv long_lived_cname_view = symtab.emplace_back(cppp::str(cname),SymbolType::FUNCTION,0,true).name();
                add_function(fid,long_lived_cname_view);
            }
            void export_function(func_id fid,Function&& fn){
                symtab.emplace_back(cppp::str(fn.cname()),SymbolType::FUNCTION,fnv.size(),false);
                add_function(fid,std::move(fn));
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

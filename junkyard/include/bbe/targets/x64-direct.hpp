#pragma once
#include"commons.hpp"
#include"../function.hpp"
#include<cppp/string.hpp> // import / export function name
#include<limits>
#include<vector>
namespace bbe::targets::x64d::impl{
    using namespace bbe::impl;
    using namespace bbe::targets::impl;
    struct Relocation{
        std::uint64_t offset;
        std::uint64_t symbol;
        std::uint64_t isize;
    };
    enum class SymbolType{
        VARIABLE,FUNCTION
    };
    struct SymbolInfo{
        cppp::str name;
        SymbolType type;
        std::uint64_t defined_begin;
        constexpr static std::uint64_t b_import_only = std::numeric_limits<std::uint64_t>::max();
    };
    class X64Program{
        std::vector<SymbolInfo> _symbols;
        std::vector<Relocation> _relocations;
        bytes _text;
        public:
            std::vector<SymbolInfo>& symbols(){
                return _symbols;
            }
            const std::vector<SymbolInfo>& symbols() const{
                return _symbols;
            }
            std::vector<Relocation>& relocations(){
                return _relocations;
            }
            const std::vector<Relocation>& relocations() const{
                return _relocations;
            }
            const bytes& text() const{
                return _text;
            }
            std::uint64_t import_symbol(cppp::str fname,SymbolType t){
                std::uint64_t sbgn = _symbols.size();
                _symbols.emplace_back(std::move(fname),t,SymbolInfo::b_import_only);
                return sbgn;
            }
            std::uint64_t start_export(cppp::str fname,SymbolType t=SymbolType::FUNCTION){
                std::uint64_t sbgn = _symbols.size();
                _symbols.emplace_back(std::move(fname),t,_text.size());
                return sbgn;
            }
            void compile(const Function&);
    };
}
namespace bbe::targets::x64d{
    BBE_EXPORT Relocation;
    BBE_EXPORT X64Program;
    BBE_EXPORT SymbolType;
    BBE_EXPORT SymbolInfo;
}

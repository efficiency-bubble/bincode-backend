#pragma once
#include"commons.hpp"
#include"type.hpp"
#include"function.hpp"
#include<cppp/strmap.hpp>
namespace bbe::impl{
    class Namespace{
        const Namespace* parent{nullptr};
        cppp::strmap<Type> _types;
        cppp::strmap<Function> _functions;
        cppp::strmap<Type> _values;
        public:
            Namespace() = default;
            Namespace(const Namespace* parent) : parent(parent){}
            const cppp::strmap<Type>& defined_types() const{
                return _types;
            }
            const cppp::strmap<Function>& defined_functions() const{
                return _functions;
            }
            const cppp::strmap<Type>& declared_values() const{
                return _values;
            }
            Type& add(str key,Type&& tp){
                return _types.try_emplace(std::move(key),std::move(tp)).first->second;
            }
            Function& add(str key,Function&& fn){
                return _functions.try_emplace(std::move(key),std::move(fn)).first->second;
            }
    };
}
namespace bbe{
    BBE_EXPORT Namespace;
}
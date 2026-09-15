#include<bbe/type.hpp>
#include<cppp/int.hpp>
namespace bbe::impl{
    const TypeInfo& TypeDatabase::pack_of(cppp::fixed_array<const TypeInfo*>&& a) const{
        type_pack key{std::move(a)};
        if(auto it=compounds.find(key);it!=compounds.end()){
            return **it;
        }else{
            TypeInfo& nt = infos.emplace(std::move(key));
            compounds.emplace(nt);
            return nt;
        }
    }
    const TypeInfo& TypeDatabase::function_of(FunctionSignature sig) const{
        if(auto it=compounds.find(sig);it!=compounds.end()){
            return **it;
        }else{
            TypeInfo& nt = infos.emplace(sig);
            compounds.emplace(nt);
            return nt;
        }
    }
    const TypeInfo& TypeDatabase::pointer_to(const TypeInfo& pointed) const{
        if(auto it=compounds.find(&pointed);it!=compounds.end()){
            return **it;
        }else{
            TypeInfo& nt = infos.emplace(&pointed);
            compounds.emplace(nt);
            return nt;
        }
    }
}

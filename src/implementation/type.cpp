#include<bbe/type.hpp>
#include<cppp/int.hpp>
namespace bbe::impl{
    const TypeInfo& TypeDatabase::pack_of(cppp::fixed_array<const TypeInfo*>&& a) const{
        type_pack key{std::move(a)};
        if(auto it=packs.find(key);it!=packs.end()){
            return *it->second;
        }else{
            std::uint64_t size = 0, align = 0;
            for(const TypeInfo* i : key.types()){
                CPPP_ASSERT(i);
                size += i->size();
                align = std::max(align,i->alignment());
            }
            TypeInfo& nt = infos.emplace(TypeCategory::PACK,size,align);
            nt.data = &inject_pack(std::move(key),nt);
            return nt;
        }
    }
    const TypeInfo& TypeDatabase::function_of(FunctionSignature sig) const{
        using namespace cppp::literals;
        if(auto it=functions.find(sig);it!=functions.end()){
            return *it->second;
        }else{
            TypeInfo& nt = infos.emplace(TypeCategory::FUNCTION_POINTER,8_u64,8_u64);
            nt.data = &inject_sig(sig,nt);
            return nt;
        }
    }
    const TypeInfo& TypeDatabase::pointer_to(const TypeInfo& pointed) const{
        using namespace cppp::literals;
        if(auto it=pointers.find(&pointed);it!=pointers.end()){
            return *it->second;
        }else{
            TypeInfo& nt = infos.emplace(TypeCategory::POINTER,8_u64,8_u64);
            nt.data = &inject_ptr(pointed,nt);
            return nt;
        }
    }
}

#include<bbe/type.hpp>
namespace bbe::impl{
    type_id TypeDatabase::pack_of(cppp::fixed_array<type_id>&& a) const{
        type_pack key{std::move(a)};
        if(auto it=packs.find(key);it!=packs.end()){
            return it->second;
        }else{
            std::uint64_t size = 0, align = 0;
            for(const type_id i : key.types()){
                CPPP_ASSERT(i != T_ERROR);
                size += infos[i].size();
                align = std::max(align,infos[i].alignment());
            }
            type_id nt = infos.emplace(FundamentalTypeType::PACK,size,align);
            infos[nt].data = &packs.try_emplace(std::move(key),nt).first->first;
            return nt;
        }
    }
    type_id TypeDatabase::function_of(FunctionSignature sig) const{
        if(auto it=functions.find(sig);it!=functions.end()){
            return it->second;
        }else{
            type_id nt = infos.emplace(FundamentalTypeType::FUNCTION_POINTER,8,8);
            infos[nt].data = &functions.try_emplace(sig,nt).first->first;
            return nt;
        }
    }
}

#include<bbe/inter/value.hpp>
namespace bbe::inter::impl{
    void stringify(const Value& v,cppp::str& s){
        using namespace std::literals;
        switch(v.tell()){
            case v.index_of<uint32v>:
                s.append(cppp::tou8(std::to_string(v.get<uint32v>().value)));
                break;
            case v.index_of<boolv>:
                if(v.get<boolv>().value){
                    s.append(u8"true"s);
                }else{
                    s.append(u8"false"s);
                }
                break;
            case v.index_of<pack>: {
                const auto& vv = v.get<pack>().values;
                s.append(u8"p["s);
                for(std::size_t i=0uz;i<vv.size();++i){
                    if(i){
                        s.push_back(u8',');
                    }
                    stringify(vv[i],s);
                }
                s.push_back(u8']');
                break;
            }
            case cppp::heap_variant<>::none:
                s.append(u8"{void}"s);
                break;
            default:
                s.append(u8"{unknown}"s);
                break;
        }
    }
}

#include"formatter.hpp"
#include<ranges>
#include<cppp/map-iter.hpp>
namespace bbe::impl{
    template<typename ...T>
    struct overloads : T...{
        using T::operator()...;
    };
    void Formatter::stringify(str& s,const SequNode& n) const{
        std::visit<void>(overloads{
            [&s](const str& kr){
                s.append(kr);
            },
            [this,&s](const std::vector<SequNode>& sq){
                for(const auto& nd : sq){
                    stringify(s,nd);
                }
            },
            [this,&s] <typename T> (const T* p){
                stringify(s,format(*p));
            }
        },n);
    }
    SequNode SimpleFormatter::format(const Type& t) const{
        std::vector<SequNode> vec;
        vec.emplace_back(u8"struct "s);
        vec.emplace_back(t.name());
        vec.emplace_back(u8" (size "s);
        vec.emplace_back(str(cppp::uview(std::to_string(t.size()))));
        vec.emplace_back(u8", align "s);
        vec.emplace_back(str(cppp::uview(std::to_string(t.alignment()))));
        vec.emplace_back(u8");\n"s);
        return {std::move(vec)};
    }
    SequNode SimpleFormatter::format(const Function& f) const{
        std::vector<SequNode> vec;
        vec.emplace_back(f.return_type()->name());
        vec.emplace_back(u8" "s);
        vec.emplace_back(f.name());
        vec.emplace_back(u8"("s);
        auto it = f.argtypes().cbegin();
        const auto end = f.argtypes().cend();
        if(it!=end){
            while(true){
                vec.emplace_back((*it)->name());
                ++it;
                if(it==end){
                    break;
                }else{
                    vec.emplace_back(u8", "s);
                }
            }
        }
        vec.emplace_back(u8"){\n"s);
        str buf;
        stringify(buf,format(f.ast()));
        vec.emplace_back(std::move(buf));
        vec.emplace_back(u8"}\n"s);
        return {std::move(vec)};
    }
    SequNode SimpleFormatter::format(const ASTNode& n) const{
        using enum ASTNode::Type;
        std::vector<SequNode> vec;
        switch(n.type()){
            case BOOLEAN:
                if(n.get_boolean()){
                    vec.emplace_back(u8"true"s);
                }else{
                    vec.emplace_back(u8"false"s);
                }
                break;
            case IF:
                vec.emplace_back(u8"if("s);
                vec.emplace_back(&n.get_condition());
                vec.emplace_back(u8")"s);
                vec.emplace_back(&n.get_conditional_body());
                break;
            case BLOCK:
                vec.emplace_back(u8"{\n"s);
                for(const ASTNode& sn : n.get_block_statements()){
                    vec.emplace_back(&sn);
                    vec.emplace_back(u8"\n"s);
                }
                vec.emplace_back(u8"}\n"s);
                break;
            default:
                vec.emplace_back(u8"/*(Unrecognized node)*/"s);
                break;
        }
        return {std::move(vec)};
    }
    SequNode SimpleFormatter::format(const Namespace& ns) const{
        std::vector<SequNode> vec;
        vec.emplace_back(u8"{\n"s);
        for(const Type& t : ns.defined_types() | cppp::map_values){
            vec.emplace_back(format(t));
        }
        for(const Function& f : ns.defined_functions() | cppp::map_values){
            vec.emplace_back(format(f));
        }
        vec.emplace_back(u8"}\n"s);
        return {std::move(vec)};
    }
}

#include<bbe/targets/mlog.hpp>
#include<unordered_map>
#include<algorithm>
#include<variant>
#include<ranges>
#include<format>
#include<print>
namespace bbe::targets::mlog::impl{
    using namespace std::literals;
    namespace ins{
        constexpr inline std::array TRANSPILE_MAP{
            u8"set"sv,u8"op"sv,u8"print"sv,u8"printflush"sv,u8"jump"sv,u8"end"sv
        };
        constexpr inline std::size_t SET = 0uz;
        constexpr inline std::size_t OP = 1uz;
        constexpr inline std::size_t PRINT = 2uz;
        constexpr inline std::size_t PRINTFLUSH = 3uz;
        constexpr inline std::size_t JUMP = 4uz;
        constexpr inline std::size_t END = 5uz;
    }
    namespace op{
        constexpr inline std::array TRANSPILE_MAP{
            u8"add"sv
        };
        constexpr inline std::size_t ADD = 0uz;
    }
    namespace jm{
        constexpr inline std::array TRANSPILE_MAP{
            u8"always"sv,u8"equal"sv
        };
        constexpr inline std::size_t ALWAYS = 0uz;
        constexpr inline std::size_t EQ = 1uz;
    }
    ProcedureIC::ProcedureIC(const ljf::ProcedureIC& ir){
        using argv = std::vector<Argument>;
        std::unordered_map<std::uint32_t,std::vector<std::uint32_t>> packs;
        auto current_label = ir.labels().rbegin();
        for(auto it=ir.instructions().begin();it!=ir.instructions().end();++it){
            if(current_label!=ir.labels().rend()&&it==*current_label){
                labels.emplace_back(instructions.size());
            }
            switch(it->opcode){
                using enum ssa::Operation;
                case IMMB:
                case IMM32:
                    instructions.emplace_back(ins::SET,argv{varref(it->dst),double(it->src.front())});
                    break;
                case PACK:
                    packs.emplace(it->dst,it->src);
                    break;
                case CMAG: {
                    switch(it->src.front()){
                        case 1550: // print
                            instructions.emplace_back(ins::PRINT,argv{varref(it->src[1uz])});
                            break;
                        case 1600: // printflush //TODO: allow selecting what to flush to
                            instructions.emplace_back(ins::PRINTFLUSH,argv{u8"message1"s});
                            break;
                        case 2520:
                            instructions.emplace_back(ins::END);
                            break;
                        default: {
                            const auto& packv = packs.at(it->src[1uz]);
                            switch(it->src.front()){
                                case 10: // add
                                    instructions.emplace_back(ins::OP,argv{operation(op::ADD),varref(it->dst),varref(packv.front()),varref(packv[1uz])});
                                    break;
                                default:
                                    throw std::logic_error("mlog::compile(): Unknown magic function "s+std::to_string(it->dst));
                            }
                            break;
                        }
                    }
                    break;
                }
                case MOV:
                    instructions.emplace_back(ins::SET,argv{varref(it->dst),varref(it->src.front())});
                    break;
                case JMP:
                    for(std::size_t i=it->src.size();i-->1uz;){
                        instructions.emplace_back(ins::JUMP,argv{label(it->src[i]),jumpmode(jm::EQ),varref(it->dst),double(i)});
                    }
                    instructions.emplace_back(ins::JUMP,argv{label(it->src.front()),jumpmode(jm::ALWAYS)});
                    break;
                default:
                    throw std::logic_error(cppp::tocs(u8"mlog::compile(): Illegal instruction "s+stringify_enum(it->opcode)));
            }
        }
    }
    cppp::str ProcedureIC::encode() const{
        cppp::str out;
        struct{
            cppp::str& out;
            const std::vector<std::uint32_t>& labels;
            void operator()(varref r) const{
                out.push_back(u8'_');
                out.append(cppp::tou8(std::to_string(r.id)));
            }
            void operator()(operation o) const{
                out.append(op::TRANSPILE_MAP[o.id]);
            }
            void operator()(label l) const{
                out.append(cppp::tou8(std::to_string(labels[l.id])));
            }
            void operator()(jumpmode o) const{
                out.append(jm::TRANSPILE_MAP[o.id]);
            }
            void operator()(double o) const{
                out.append(cppp::tou8(std::to_string(o)));
            }
            void operator()(const cppp::str& o) const{
                out.append(o);
            }
        } encode{out,labels};
        for(const auto& ins : instructions){
            out.append(ins::TRANSPILE_MAP[ins.ins_id]);
            for(const Argument& arg : ins.argv){
                out.push_back(u8' ');
                arg.visit(encode);
            }
            out.push_back('\n');
        }
        return out;
    }
}

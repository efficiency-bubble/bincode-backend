#include<bbe/targets/mlog.hpp>
#include<unordered_map>
#include<algorithm>
#include<format>
namespace bbe::targets::mlog::impl{
    using namespace std::literals;
    cppp::str compile(const ljf::ProcedureIC& ir){
        cppp::str code;
        std::unordered_map<std::uint32_t,std::vector<std::uint32_t>> packs;
        std::vector<std::uint32_t> labels{ir.labels()};
        std::ranges::reverse(labels);
        std::uint32_t ins_mlog = 0;
        std::uint32_t ins_ljf = 0;
        for(const auto& ins : ir.instructions()){
            switch(ins.opcode){
                using enum ssa::Operation;
                case IMM32:
                    code.append(cppp::tou8(std::format("set _{} {}\n"sv,ins.dst,ins.src.front())));
                    ++ins_mlog;
                    break;
                case PACK:
                    packs.emplace(ins.dst,ins.src);
                    break;
                case CMAG: {
                    const auto& packv = packs.at(ins.src.back());
                    switch(ins.src.front()){
                        case 10: // add
                            code.append(cppp::tou8(std::format("op add _{} _{} _{}\n"sv,ins.dst,packv.front(),packv.back())));
                            ++ins_mlog;
                            break;
                        default:
                            throw std::logic_error("mlog::compile(): Unknown magic function "s+std::to_string(ins.dst));
                    }
                    break;
                }
                default:
                    throw std::logic_error(cppp::tocs(u8"mlog::compile(): Illegal instruction "s+stringify_enum(ins.opcode)));
            }
            ++ins_ljf;
        }
        return code;
    }
}

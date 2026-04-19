#include<bbe/targets/x86.hpp>
#include<assembly/instruction.hpp>
#include<unordered_map>
#include<cppp/int.hpp>
#include<vector>
namespace bbe::targets::x86::impl{
    namespace x = ::x86;
    using namespace std::literals;
    using namespace cppp::literals;
    constexpr static std::uint32_t NVAL = std::numeric_limits<std::uint32_t>::max();
    class DataValue{
        std::uint32_t _v;
        bool pack;
        public:
            DataValue() : _v(NVAL), pack(false){}
            DataValue(std::uint32_t v,bool pack) : _v(v), pack(pack){}
            bool is_pack() const{
                return pack;
            }
            std::uint32_t id() const{
                return _v;
            }
    };
    namespace{
        class FunctionCompiler{
            public:
                using value_t = std::uint32_t;
            private:
                Function& f;
                value_t value_counter = 0;
                std::uint32_t sp = 0;
                std::unordered_map<std::uint32_t,std::vector<value_t>> pack_contents;
                std::unordered_map<value_t,std::uint32_t> value_frame_off;
                value_t new_value_id(){
                    return value_counter++;
                }
                std::uint32_t allocate_dw(value_t vid){
                    sp += 4;
                    value_frame_off.try_emplace(vid,sp);
                    return sp;
                }
                std::uint32_t allocate_qw(value_t vid){
                    sp += 8;
                    value_frame_off.try_emplace(vid,sp);
                    return sp;
                }
                static x::displacement<x::width::W8> soff_to_disp8(std::uint32_t off){
                    if(off > 0xFF) throw std::logic_error("x86 compile: soff_to_disp8: stack offset too large");
                    return {static_cast<std::int8_t>(-static_cast<std::int32_t>(off))};
                }
                static value_t require_value(DataValue dv,const cppp::str& orelse){
                    if(dv.is_pack()){
                        throw std::logic_error(reinterpret_cast<const char*>(orelse.c_str()));
                    }
                    return dv.id();
                }
                static std::uint32_t require_pack(DataValue dv,const cppp::str& orelse){
                    if(dv.is_pack()){
                        return dv.id();
                    }else{
                        throw std::logic_error(reinterpret_cast<const char*>(orelse.c_str()));
                    }
                }
                template<x::width w=x::width::W32>
                static void rtos(cppp::bytes& b,std::byte r,x::displacement<x::width::W8> soff){
                    x::instructions::mov::rm_r::for_width<w>::encode(b,0b01_b,x::reg::BP,soff,r);
                }
                static void stor(cppp::bytes& b,x::displacement<x::width::W8> soff,std::byte r){
                    x::instructions::mov::r_rm::for_width<x::width::W32>::encode(b,r,0b01_b /* disp8 */,x::reg::BP,soff);
                }
                template<typename T>
                DataValue encode_arith(const dfg::DataNode& lhsn,const dfg::DataNode& rhsn){
                    value_t vid = new_value_id();
                    value_t lhs = require_value(compile_node(lhsn),u8"x86 compile: lhs to arithmetic op is erroneously a pack"s);
                    value_t rhs = require_value(compile_node(rhsn),u8"x86 compile: rhs to arithmetic op is erroneously a pack"s);
                    into(lhs,x::reg::A);
                    into(rhs,x::reg::C);
                    T::rm_r::template for_width<x::width::W32>::encode(f.instructions(),0b11_b,x::reg::A,x::reg::C);
                    rtos(f.instructions(),x::reg::A,soff_to_disp8(allocate_dw(vid)));
                    return {vid,false};
                }
                template<typename T>
                DataValue encode_comparison(const dfg::DataNode& lhsn,const dfg::DataNode& rhsn){
                    value_t vid = new_value_id();
                    value_t lhs = require_value(compile_node(lhsn),u8"x86 compile: lhs to compare is erroneously a pack"s);
                    value_t rhs = require_value(compile_node(rhsn),u8"x86 compile: rhs to compare is erroneously a pack"s);
                    into(lhs,x::reg::A);
                    into(rhs,x::reg::C);
                    x::instructions::cmp::rm_r::for_width<x::width::W32>::encode(f.instructions(),0b11_b,x::reg::A,x::reg::C);
                    T::encode(f.instructions(),0b11_b,x::reg::A);
                    x::instructions::movzx::from_b::for_width<x::width::W32>::encode(f.instructions(),x::reg::A,0b11_b,x::reg::A);
                    rtos(f.instructions(),x::reg::A,soff_to_disp8(allocate_dw(vid)));
                    return {vid,false};
                }
                static std::byte arg_reg(std::uint32_t ind){
                    switch(ind){
                        case 0: return x::reg::DI;
                        case 1: return x::reg::SI;
                        case 2: return x::reg::D;
                        case 3: return x::reg::C;
                        default: throw std::logic_error("x86 compile: Don't know where is argument #"s+std::to_string(ind));
                    }
                }
            public:
                FunctionCompiler(Function& f) : f(f){}
                void into(value_t v,std::byte reg) const{
                    stor(f.instructions(),soff_to_disp8(value_frame_off.at(v)),reg);
                }
                void load_args(type_id t,const TypeDatabase& tdb){
                    value_t pid = new_value_id();
                    if(pid) throw std::logic_error("x86 compile: the argument, if present, must be loaded first"s);
                    if(tdb[t].type() != FundamentalTypeType::UNSIGNED_INTEGRAL) throw std::logic_error("x86 compile: ABI: argument must be integral"s);
                    rtos(f.instructions(),arg_reg(0),soff_to_disp8(allocate_dw(pid)));
                }
                DataValue compile_node(const dfg::DataNode& dn){
                    switch(dn.operation()){
                        using enum dfg::NodeType;
                        case UINT32: {
                            value_t vid = new_value_id();
                            x::instructions::mov::rm_imm::for_width<x::width::W32>::encode(f.instructions(),0b01_b /* disp8 */,x::reg::BP,soff_to_disp8(allocate_dw(vid)),dn.primitive());
                            return {vid,false};
                        }
                        case PACK: {
                            value_t pid = new_value_id();
                            auto& contents = pack_contents.try_emplace(pid).first->second;
                            for(const dfg::DataNode* child : dn.parents()){
                                contents.emplace_back(require_value(compile_node(*child),u8"x86 compile: nested packs are unsupported"s));
                            }
                            return {pid,true};
                        }
                        case PACKIND: {
                            std::uint32_t pkid = require_pack(compile_node(*dn.parents().front()),u8"x86 compile: can't index a non-pack"s);
                            return {pack_contents[pkid][dn.primitive()],false};
                        }
                        case ARG:
                            return {0,false};
                        case CALL_BUILTIN: {
                            switch(dn.primitive()){
                                case 0: {
                                    value_t ret = new_value_id();
                                    value_t fn = require_value(compile_node(*dn.parents().front()),u8"x86 compile: a pack is not a function pointer"s);
                                    value_t argv = require_value(compile_node(*dn.parents()[1]),u8"x86 compile: ABI: can't pass a pack as argument"s);
                                    stor(f.instructions(),soff_to_disp8(value_frame_off.at(argv)),arg_reg(0));
                                    x::instructions::call::near_abs::for_width<x::width::W64>::encode(f.instructions(),0x01_b /* disp8 */,x::reg::BP,soff_to_disp8(value_frame_off.at(fn)));
                                    rtos(f.instructions(),x::reg::A,soff_to_disp8(allocate_dw(ret)));
                                    return {ret,false};
                                }
                                case 10:
                                case 11:
                                    return encode_arith<x::instructions::add>(*dn.parents()[0uz],*dn.parents()[1uz]);
                                case 20:
                                case 21:
                                    return encode_arith<x::instructions::sub>(*dn.parents()[0uz],*dn.parents()[1uz]);
                                case 50:
                                    return encode_comparison<x::instructions::set::e>(*dn.parents()[0uz],*dn.parents()[1uz]);
                                case 51:
                                    return encode_comparison<x::instructions::set::le>(*dn.parents()[0uz],*dn.parents()[1uz]);
                                default:
                                    throw std::logic_error("x86 compile: unknown magic "s+std::to_string(dn.primitive()));
                            }
                        }
                        case BOOL: {
                            value_t vid = new_value_id();
                            x::instructions::mov::rm_imm::for_width<x::width::W32>::encode(f.instructions(),0b01_b /* disp8 */,x::reg::BP,soff_to_disp8(allocate_dw(vid)),dn.primitive());
                            return {vid,false};
                        }
                        case FORK: {
                            value_t rid = new_value_id();
                            x::displacement<x::width::W8> spoff = soff_to_disp8(allocate_dw(rid));
                            value_t cond = require_value(compile_node(*dn.parents()[0uz]),u8"x86 compile: condition in fork is erroneously a pack"s);
                            into(cond,x::reg::A);
                            x::instructions::test::rm_r::for_width<x::width::W32>::encode(f.instructions(),0b11_b,x::reg::A,x::reg::A);
                            std::size_t jzloc = f.instructions().size();
                            auto jz = x::instructions::j::z::for_width<x::width::W8>::encode(f.instructions(),x::skip_immediate);
                            {
                                value_t lv = require_value(compile_node(*dn.parents()[1uz]),u8"x86 compile: unsupported: lhs of fork being a pack"s);
                                into(lv,x::reg::A);
                                rtos(f.instructions(),x::reg::A,spoff);
                            }
                            std::size_t jeloc = f.instructions().size();
                            auto je = x::instructions::jmp::rel::for_width<x::width::W8>::encode(f.instructions(),x::skip_immediate);
                            
                            cppp::write<std::int8_t>(f.instructions().data()+jzloc+jz.offset_of_first<x::ComponentType::IMMEDIATE>,static_cast<std::int8_t>(f.instructions().size()-jzloc-jz.total_size));
                            {
                                value_t rv = require_value(compile_node(*dn.parents()[2uz]),u8"x86 compile: unsupported: rhs of fork being a pack"s);
                                into(rv,x::reg::A);
                                rtos(f.instructions(),x::reg::A,spoff);
                            }
                            cppp::write<std::int8_t>(f.instructions().data()+jeloc+je.offset_of_first<x::ComponentType::IMMEDIATE>,static_cast<std::int8_t>(f.instructions().size()-jeloc-je.total_size));
                            return {rid,false};
                        }
                        case FNSYM: {
                            value_t rid = new_value_id();
                            std::size_t offs = f.instructions().size();
                            auto lea = x::instructions::lea::for_width<x::width::W64>::encode(f.instructions(),x::reg::A,0b00_b,0b101_b /* rip+disp32 on 64-bit mode*/,x::skip_displacement<x::width::W32>);
                            constexpr std::size_t disp_local_offs = lea.offset_of_first<x::ComponentType::DISPLACEMENT>;
                            offs += disp_local_offs;
                            f.add_relocation({.offset=static_cast<std::uint32_t>(offs),.fni=dn.primitive(),.isize=static_cast<std::uint32_t>(lea.total_size-disp_local_offs)});
                            rtos<x::width::W64>(f.instructions(),x::reg::A,soff_to_disp8(allocate_qw(rid)));
                            return {rid,false};
                        }
                        case STDOUT:
                            return {};
                        default:
                            throw std::logic_error("x86 compile: unknown op "s+std::to_string(std::to_underlying(dn.operation())));
                    }
                }
                std::uint32_t stack_size() const{
                    return sp;
                }
        };
    }
    Function::Function(const dfg::Function& f,const TypeDatabase& tdb){
        FunctionCompiler compiler{*this};
        x::instructions::push::r64(b,x::reg::BP);
        x::instructions::mov::rm_r::for_width<x::width::W64>::encode(b,0b11_b,x::reg::BP,x::reg::SP);
        std::size_t enter = b.size();
        enter += x::instructions::sub::rm_imm::for_width<x::width::W64>::encode(b,0b11_b,x::reg::SP,x::skip_immediate).offset_of_first<x::ComponentType::IMMEDIATE>;
        compiler.load_args(f.signature().parameter(),tdb);
        compiler.compile_node(*f.dfg().stdout_result());
        auto retv{compiler.compile_node(*f.dfg().root())};
        if(retv.is_pack()){
            throw std::logic_error("x86 compile: ABI: must not return a pack");
        }
        compiler.into(retv.id(),x::reg::A);
        x::instructions::leave(b);
        x::instructions::ret::near(b);
        cppp::write<std::uint32_t>(b.data()+enter,compiler.stack_size());
    }
}

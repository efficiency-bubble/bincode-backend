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
                std::uint32_t allocate_dw(std::uint32_t vid){
                    sp += 4;
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
                static void rtos(cppp::bytes& b,std::byte r,x::displacement<x::width::W8> soff){
                    x::instructions::mov::rm_r::for_width<x::width::W32>::encode(b,0b01_b,0b101_b,soff,r);
                }
                static void stor(cppp::bytes& b,std::int8_t soff,std::byte r){
                    x::instructions::mov::r_rm::for_width<x::width::W32>::encode(b,r,0b01_b,0b101_b /* BP + disp8 */,x::displacement<x::width::W8>(-soff));
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
            public:
                FunctionCompiler(Function& f) : f(f){}
                void into(value_t v,std::byte reg) const{
                    stor(f.instructions(),static_cast<std::int8_t>(value_frame_off.at(v)),reg);
                }
                DataValue compile_node(const dfg::DataNode& dn){
                    switch(dn.operation()){
                        using enum dfg::NodeType;
                        case UINT32: {
                            value_t vid = new_value_id();
                            x::instructions::mov::rm_imm::for_width<x::width::W32>::encode(f.instructions(),0b01_b,0b101_b /* BP + disp8 */,soff_to_disp8(allocate_dw(vid)),dn.primitive());
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
                        case CALL_BUILTIN: {
                            switch(dn.primitive()){
                                case 10:
                                    return encode_arith<x::instructions::add>(*dn.parents()[0uz],*dn.parents()[1uz]);
                                case 11:
                                    return encode_arith<x::instructions::sub>(*dn.parents()[0uz],*dn.parents()[1uz]);
                                case 50:
                                    return encode_comparison<x::instructions::set::e>(*dn.parents()[0uz],*dn.parents()[1uz]);
                                case 51:
                                    return encode_comparison<x::instructions::set::le>(*dn.parents()[0uz],*dn.parents()[1uz]);
                                default:
                                    throw std::logic_error("x86 compile: unknown magic "s+std::to_string(dn.primitive()));
                            }
                        }
                        case STDOUT:
                            return {};
                        default:
                            throw std::logic_error("x86 compile: unknown op "s+std::to_string(std::to_underlying(dn.operation())));
                    }
                }
        };
    }
    Function::Function(const dfg::DataFlowGraph& dfg){
        FunctionCompiler compiler{*this};
        x::instructions::push::r64(b,x::reg::BP);
        x::instructions::mov::rm_r::for_width<x::width::W64>::encode(b,0b11_b,x::reg::BP,x::reg::SP);
        compiler.compile_node(*dfg.stdout_result());
        auto retv{compiler.compile_node(*dfg.root())};
        if(retv.is_pack()){
            throw std::logic_error("x86 compile: ABI: must not return a pack");
        }
        compiler.into(retv.id(),x::reg::A);
        x::instructions::leave(b);
        x::instructions::ret::near(b);
    }
}

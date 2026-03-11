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
        template<typename Ins,x::width ...Disp> requires(sizeof...(Disp) < 2uz)
        void encode_rm_r(x::width w,cppp::bytes& b,std::byte mod,std::byte rm,std::byte r,x::wvs<Disp>... disp){
            x::Instruction i{Ins::rm_r};
            i.set_width(w);
            i.mod_rm(mod,rm);
            if constexpr(sizeof...(disp)){
                i.displacement(disp...);
            }
            i.rm_reg(r);
            i.encode(b);
        }
        template<typename Ins,x::width w,x::width ...Disp> requires(sizeof...(Disp) < 2uz)
        void encode_rm_i(cppp::bytes& b,std::byte mod,std::byte rm,x::wv<w> imm,x::wvs<Disp>... disp){
            x::Instruction i{Ins::rm_imm};
            i.set_width(w);
            i.mod_rm(mod,rm);
            i.immediate(imm);
            if constexpr(sizeof...(disp)){
                i.displacement(disp...);
            }
            i.encode(b);
        }
        template<typename Ins,x::width ...Disp> requires(sizeof...(Disp) < 2uz)
        void encode_r_rm(x::width w,cppp::bytes& b,std::byte r,std::byte mod,std::byte rm,x::wvs<Disp>... disp){
            x::Instruction i{Ins::r_rm};
            i.set_width(w);
            i.mod_rm(mod,rm);
            if constexpr(sizeof...(disp)){
                i.displacement(disp...);
            }
            i.rm_reg(r);
            i.encode(b);
        }
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
                static value_t require_value(DataValue dv,const cppp::str& orelse){
                    if(dv.is_pack()){
                        throw std::logic_error(reinterpret_cast<const char*>(orelse.c_str()));
                    }
                    return dv.id();
                }
                static void rtos(cppp::bytes& b,std::byte r,std::int8_t soff){
                    encode_rm_r<x::encode::mov,x::width::W8>(x::width::W32,b,0b01_b,0b101_b /* BP + disp8 */,r,-soff);
                }
                static void stor(cppp::bytes& b,std::int8_t soff,std::byte r){
                    encode_r_rm<x::encode::mov,x::width::W8>(x::width::W32,b,r,0b01_b,0b101_b /* BP + disp8 */,-soff);
                }
                template<typename T>
                DataValue encode_arith(const dfg::DataNode& lhsn,const dfg::DataNode& rhsn){
                    value_t vid = new_value_id();
                    value_t lhs = require_value(compile_node(lhsn),u8"x86 compile: lhs to arithmetic op is erroneously a pack"s);
                    value_t rhs = require_value(compile_node(rhsn),u8"x86 compile: rhs to arithmetic op is erroneously a pack"s);
                    into(lhs,x::reg::A);
                    into(rhs,x::reg::C);
                    encode_rm_r<T>(x::width::W32,f.instructions(),0b11_b,x::reg::A,x::reg::C);
                    rtos(f.instructions(),x::reg::A,static_cast<std::int8_t>(allocate_dw(vid)));
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
                            encode_rm_i<x::encode::mov,x::width::W32,x::width::W8>(f.instructions(),0b01_b,0b101_b /* BP + disp8 */,dn.primitive(),-static_cast<std::int8_t>(allocate_dw(vid)));
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
                                    return encode_arith<x::encode::add>(*dn.parents()[0uz],*dn.parents()[1uz]);
                                case 11:
                                    return encode_arith<x::encode::sub>(*dn.parents()[0uz],*dn.parents()[1uz]);
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
        x::encode::push::r64(b,x::reg::BP);
        encode_rm_r<x::encode::mov>(x::width::W64,b,0b11_b,x::reg::BP,x::reg::SP);
        compiler.compile_node(*dfg.stdout_result());
        auto retv{compiler.compile_node(*dfg.root())};
        if(retv.is_pack()){
            throw std::logic_error("x86 compile: ABI: must not return a pack");
        }
        compiler.into(retv.id(),x::reg::A);
        x::encode::leave(b);
        x::encode::ret::near(b);
    }
}

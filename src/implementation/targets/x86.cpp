#include<assembly/instruction.hpp>
#include<bbe/targets/x86.hpp>
#include<unordered_map>
#include<cppp/int.hpp>
#include<vector>
namespace bbe::targets::x86::impl{
    namespace x = ::x86;
    using namespace std::literals;
    using namespace cppp::literals;
    constexpr static std::uint32_t NSOFF = std::numeric_limits<std::uint32_t>::max();
    namespace{
        class DataValue{
            std::vector<const DataValue*> _pack;
            const TypeInfo* _type;
            std::uint32_t soff = NSOFF;
            public:
                DataValue(const TypeInfo& t) : _type(&t){}
                bool is_pack() const{
                    return _type->type() == TypeCategory::PACK;
                }
                const TypeInfo& type() const{
                    return *_type;
                }
                const std::vector<const DataValue*>& pack_contents() const{
                    CPPP_ASSERT(is_pack());
                    return _pack;
                }
                std::vector<const DataValue*>& pack_contents(){
                    CPPP_ASSERT(is_pack());
                    return _pack;
                }
                void set_stack(std::uint32_t off){
                    soff = off;
                }
                std::uint32_t stack() const{
                    return soff;
                }
        };
        class FunctionCompiler{
            public:
                using value_t = std::uint32_t;
            private:
                Function& f;
                const TypeDatabase& tdb;
                value_t value_counter = 0;
                std::uint32_t sp = 0;
                std::unordered_map<value_t,DataValue> values;
                DataValue& new_value(const TypeInfo& t){
                    return values.try_emplace(value_counter++,t).first->second;
                }
                DataValue& new_value(type_id t){
                    return new_value(tdb[t]);
                }
                std::uint32_t allocate_stack(std::uint32_t sz,DataValue& v){
                    sp += sz;
                    v.set_stack(sp);
                    return sp;
                }
                std::uint32_t allocate_dw(DataValue& v){
                    return allocate_stack(4,v);
                }
                std::uint32_t allocate_qw(DataValue& v){
                    return allocate_stack(8,v);
                }
                static x::displacement<x::width::W8> soff_to_disp8(std::uint32_t off){
                    CPPP_ASSERT(off != NSOFF);
                    if(off > 0xFF) throw std::logic_error("x86 compile: soff_to_disp8: stack offset too large");
                    return {static_cast<std::int8_t>(-static_cast<std::int32_t>(off))};
                }
                template<x::width w=x::width::W32>
                static void rtos(cppp::bytes& b,std::byte r,x::displacement<x::width::W8> soff){
                    x::instructions::mov::rm_r::for_width<w>::encode(b,0b01_b,x::reg::BP,soff,r);
                }
                static void rtosd(std::uint32_t sz,cppp::bytes& b,std::byte r,x::displacement<x::width::W8> soff){
                    switch(sz){
                        case 1: rtos<x::width::W8>(b,r,soff); break;
                        case 2: rtos<x::width::W16>(b,r,soff); break;
                        case 4: rtos<x::width::W32>(b,r,soff); break;
                        case 8: rtos<x::width::W64>(b,r,soff); break;
                        default: std::unreachable();
                    }
                }
                template<x::width w=x::width::W32>
                static void stor(cppp::bytes& b,x::displacement<x::width::W8> soff,std::byte r){
                    x::instructions::mov::r_rm::for_width<w>::encode(b,r,0b01_b /* disp8 */,x::reg::BP,soff);
                }
                static void stord(std::uint32_t sz,cppp::bytes& b,x::displacement<x::width::W8> soff,std::byte r){
                    switch(sz){
                        case 1: stor<x::width::W8>(b,soff,r); break;
                        case 2: stor<x::width::W16>(b,soff,r); break;
                        case 4: stor<x::width::W32>(b,soff,r); break;
                        case 8: stor<x::width::W64>(b,soff,r); break;
                        default: std::unreachable();
                    }
                }
                template<typename T>
                DataValue& encode_arith(const TypeInfo& rt,const dfg::DataNode& lhsn,const dfg::DataNode& rhsn){
                    DataValue& rv = new_value(rt);
                    const DataValue& lhs = *compile_node(lhsn);
                    const DataValue& rhs = *compile_node(rhsn);
                    into(lhs,x::reg::A);
                    into(rhs,x::reg::C);
                    T::rm_r::template for_width<x::width::W32>::encode(f.instructions(),0b11_b,x::reg::A,x::reg::C);
                    rtos(f.instructions(),x::reg::A,soff_to_disp8(allocate_dw(rv)));
                    return rv;
                }
                template<typename T>
                DataValue& encode_comparison(const TypeInfo& rt,const dfg::DataNode& lhsn,const dfg::DataNode& rhsn){
                    DataValue& rv = new_value(rt);
                    const DataValue& lhs = *compile_node(lhsn);
                    const DataValue& rhs = *compile_node(rhsn);
                    into(lhs,x::reg::A);
                    into(rhs,x::reg::C);
                    x::instructions::cmp::rm_r::for_width<x::width::W32>::encode(f.instructions(),0b11_b,x::reg::A,x::reg::C);
                    T::encode(f.instructions(),0b11_b,x::reg::A);
                    x::instructions::movzx::from_b::for_width<x::width::W32>::encode(f.instructions(),x::reg::A,0b11_b,x::reg::A);
                    rtos(f.instructions(),x::reg::A,soff_to_disp8(allocate_dw(rv)));
                    return rv;
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
                FunctionCompiler(Function& f,const TypeDatabase& tdb) : f(f), tdb(tdb){}
                void into(const DataValue& v,std::byte reg) const{
                    stor(f.instructions(),soff_to_disp8(v.stack()),reg);
                }
                void load_args(const TypeInfo& argt){
                    DataValue& argv = new_value(argt);
                    if(argt.type() == TypeCategory::VOID) return; // nothing here
                    else if(argt.type() == TypeCategory::PACK){
                        auto& contents = argv.pack_contents();
                        for(std::uint32_t i=0;i<argt.pack_contents().types().size();++i){
                            const TypeInfo& arg_i_t = *argt.pack_contents().types()[i];
                            if(arg_i_t.type() == TypeCategory::VOID) continue;
                            else if(arg_i_t.type() == TypeCategory::PACK) throw std::logic_error("x86 compile: ABI: Can't have packs in an argument pack"s);
                            DataValue& arg_i_v = new_value(arg_i_t);
                            
                            std::uint32_t asize = static_cast<std::uint32_t>(arg_i_t.size());
                            rtosd(asize,f.instructions(),arg_reg(i),soff_to_disp8(allocate_stack(asize,arg_i_v)));
                            contents.emplace_back(&arg_i_v);
                        }
                    }else{
                        rtos(f.instructions(),arg_reg(0),soff_to_disp8(allocate_stack(static_cast<std::uint32_t>(argt.size()),argv)));
                    }
                }
                const DataValue* compile_node(const dfg::DataNode& dn){
                    switch(dn.operation()){
                        using enum dfg::NodeType;
                        case UINT32: {
                            DataValue& v = new_value(dn.return_type());
                            x::instructions::mov::rm_imm::for_width<x::width::W32>::encode(f.instructions(),0b01_b /* disp8 */,x::reg::BP,soff_to_disp8(allocate_dw(v)),dn.primitive());
                            return &v;
                        }
                        case PACK: {
                            DataValue& v = new_value(dn.return_type());
                            for(const dfg::DataNode* child : dn.parents()){
                                v.pack_contents().emplace_back(compile_node(*child));
                            }
                            return &v;
                        }
                        case PACKIND:
                            return compile_node(*dn.parents().front())->pack_contents()[dn.primitive()];
                        case ARG:
                            return &values.at(0);
                        case CALL_BUILTIN: {
                            switch(dn.primitive()){
                                case 0: {
                                    DataValue& ret = new_value(dn.return_type());
                                    const DataValue& fn = *compile_node(*dn.parents()[0uz]);
                                    
                                    const DataValue& arg = *compile_node(*dn.parents()[1uz]);
                                    if(arg.is_pack()){
                                        for(std::uint32_t i=0;i<arg.pack_contents().size();++i){
                                            stord(static_cast<std::uint32_t>(arg.pack_contents()[i]->type().size()),f.instructions(),soff_to_disp8(arg.pack_contents()[i]->stack()),arg_reg(i));
                                        }
                                    }else{
                                        stor(f.instructions(),soff_to_disp8(arg.stack()),arg_reg(0));
                                    }
                                    
                                    x::instructions::call::near_abs::for_width<x::width::W64>::encode(f.instructions(),0x01_b /* disp8 */,x::reg::BP,soff_to_disp8(fn.stack()));
                                    rtos(f.instructions(),x::reg::A,soff_to_disp8(allocate_dw(ret)));
                                    return &ret;
                                }
                                case 10:
                                case 11:
                                    return &encode_arith<x::instructions::add>(tdb[dn.return_type()],*dn.parents()[0uz],*dn.parents()[1uz]);
                                case 20:
                                case 21:
                                    return &encode_arith<x::instructions::sub>(tdb[dn.return_type()],*dn.parents()[0uz],*dn.parents()[1uz]);
                                case 30:{
                                    DataValue& ret = new_value(dn.return_type());
                                    
                                    const DataValue& lhs = *compile_node(*dn.parents()[0uz]);
                                    const DataValue& rhs = *compile_node(*dn.parents()[1uz]);
                                    into(lhs,x::reg::A);
                                    into(rhs,x::reg::C);
                                    x::instructions::imul::for_width<x::width::W32>::encode(f.instructions(),0b11_b,x::reg::A);
                                    rtos(f.instructions(),x::reg::A,soff_to_disp8(allocate_dw(ret)));
                                    return &ret;
                                }
                                case 50:
                                    return &encode_comparison<x::instructions::set::e>(tdb[dn.return_type()],*dn.parents()[0uz],*dn.parents()[1uz]);
                                case 51:
                                    return &encode_comparison<x::instructions::set::le>(tdb[dn.return_type()],*dn.parents()[0uz],*dn.parents()[1uz]);
                                default:
                                    throw std::logic_error("x86 compile: unknown magic "s+std::to_string(dn.primitive()));
                            }
                        }
                        case BOOL: {
                            DataValue& v = new_value(dn.return_type());
                            x::instructions::mov::rm_imm::for_width<x::width::W32>::encode(f.instructions(),0b01_b /* disp8 */,x::reg::BP,soff_to_disp8(allocate_dw(v)),dn.primitive());
                            return &v;
                        }
                        case FORK: {
                            DataValue& rv = new_value(dn.return_type());
                            x::displacement<x::width::W8> spoff = soff_to_disp8(allocate_dw(rv));
                            const DataValue& cond = *compile_node(*dn.parents()[0uz]);
                            into(cond,x::reg::A);
                            x::instructions::test::rm_r::for_width<x::width::W32>::encode(f.instructions(),0b11_b,x::reg::A,x::reg::A);
                            std::size_t jzloc = f.instructions().size();
                            auto jz = x::instructions::j::z::for_width<x::width::W8>::encode(f.instructions(),x::skip_immediate);
                            {
                                const DataValue& lhv = *compile_node(*dn.parents()[1uz]);
                                into(lhv,x::reg::A);
                                rtos(f.instructions(),x::reg::A,spoff);
                            }
                            std::size_t jeloc = f.instructions().size();
                            auto je = x::instructions::jmp::rel::for_width<x::width::W8>::encode(f.instructions(),x::skip_immediate);
                            
                            cppp::write<std::int8_t>(f.instructions().data()+jzloc+jz.offset_of_first<x::ComponentType::IMMEDIATE>,static_cast<std::int8_t>(f.instructions().size()-jzloc-jz.total_size));
                            {
                                const DataValue& rhv = *compile_node(*dn.parents()[2uz]);
                                into(rhv,x::reg::A);
                                rtos(f.instructions(),x::reg::A,spoff);
                            }
                            cppp::write<std::int8_t>(f.instructions().data()+jeloc+je.offset_of_first<x::ComponentType::IMMEDIATE>,static_cast<std::int8_t>(f.instructions().size()-jeloc-je.total_size));
                            return &rv;
                        }
                        case FNSYM: {
                            DataValue& rv = new_value(dn.return_type());
                            std::size_t offs = f.instructions().size();
                            auto lea = x::instructions::lea::for_width<x::width::W64>::encode(f.instructions(),x::reg::A,0b00_b,0b101_b /* rip+disp32 on 64-bit mode */,x::skip_displacement<x::width::W32>);
                            constexpr std::size_t disp_local_offs = lea.offset_of_first<x::ComponentType::DISPLACEMENT>;
                            offs += disp_local_offs;
                            f.add_relocation({.offset=static_cast<std::uint32_t>(offs),.fni=dn.primitive(),.isize=static_cast<std::uint32_t>(lea.total_size-disp_local_offs)});
                            rtos<x::width::W64>(f.instructions(),x::reg::A,soff_to_disp8(allocate_qw(rv)));
                            return &rv;
                        }
                        case STDOUT:
                            return nullptr;
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
        FunctionCompiler compiler{*this,tdb};
        x::instructions::push::r64(b,x::reg::BP);
        x::instructions::mov::rm_r::for_width<x::width::W64>::encode(b,0b11_b,x::reg::BP,x::reg::SP);
        std::size_t enter = b.size();
        enter += x::instructions::sub::rm_imm::for_width<x::width::W64>::encode(b,0b11_b,x::reg::SP,x::skip_immediate).offset_of_first<x::ComponentType::IMMEDIATE>;
        compiler.load_args(*f.signature().parameter());
        compiler.compile_node(*f.dfg().stdout_result());
        const DataValue& retv{*compiler.compile_node(*f.dfg().root())};
        if(retv.is_pack()){
            throw std::logic_error("x86 compile: ABI: must not return a pack");
        }
        compiler.into(retv,x::reg::A);
        x::instructions::leave(b);
        x::instructions::ret::near(b);
        cppp::write<std::uint32_t>(b.data()+enter,compiler.stack_size());
    }
}

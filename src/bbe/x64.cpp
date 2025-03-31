#include"assembly.hpp"
#include<cppp/polyfill/pack-indexing.hpp>
#include<assembly/instruction.hpp>
#include<cppp/freelist.hpp>
#include<type_traits>
#include<optional>
#include<variant>
#include<cstdio>
#include<map>
namespace bbe::impl{
    namespace x64{
        class FunctionCompilationContext{
            bytes _text;
            constexpr static std::array<x86::Reg,3> GENERIC_REGS{x86::reg::A,x86::reg::C,x86::reg::D};
            class Value{
                public:
                    enum Kind : std::uint8_t{
                        STACK = 0, REG = 1, CST = 2
                    };
                    template<Kind k>
                    using type_of = cppp::compat::index_pack<
                        static_cast<std::size_t>(k),
                        std::uint32_t,
                        x86::Reg,
                        std::uint32_t
                    >;
                private:
                    template<Kind ...k>
                    struct _variant_of_these{
                        using type = std::variant<type_of<k>...>;
                    };
                    using data_t = _variant_of_these<Kind::STACK,Kind::REG,Kind::CST>::type;
                    data_t _data;
                    Value(data_t d) : _data(std::move(d)){}
                public:
                    template<Kind k>
                    static Value construct(type_of<k> value){
                        return data_t(std::in_place_index<static_cast<std::size_t>(k)>,value);
                    }
                    template<Kind k>
                    type_of<k> get() const{
                        return std::get<static_cast<std::size_t>(k)>(_data);
                    }
                    Kind kind() const{
                        return static_cast<Kind>(_data.index());
                    }
                    template<Kind k>
                    void set(type_of<k> value){
                        _data.emplace<static_cast<std::size_t>(k)>(value);
                    }
            };
            cppp::freelist<> id_list;
            cppp::freelist<> stack;
            using vals_t = std::map<std::uint64_t,Value>;
            vals_t values;
            struct reg_less{
                constexpr static bool operator()(x86::Reg lhs,x86::Reg rhs){
                    return static_cast<std::uint8_t>(lhs.value()) < static_cast<std::uint8_t>(rhs.value());
                }
            };
            std::map<x86::Reg,std::uint64_t,reg_less> used_regs;
            void reg_evict(x86::Reg r){
                auto node{used_regs.extract(r)};
                if(node){
                    values.at(node.mapped()).set<Value::STACK>(stack.allocate());
                }
            }
            x86::Reg free_reg(){
                for(const x86::Reg r : GENERIC_REGS){
                    if(!used_regs.contains(r)){
                        return r;
                    }
                }
                reg_evict(GENERIC_REGS.front());
                return GENERIC_REGS.front();
            }
            template<Value::Kind k>
            void store_v_d(Value& v,x86::Reg d){
                insd<x86::encode::mov,Value::REG,k>(d,v.get<k>());
                v.set<Value::REG>(d);
            }
            template<Value::Kind k>
            x86::Reg store_v(Value& v){
                x86::Reg reg{free_reg()};
                store_v_d<k>(v,reg);
                return reg;
            }
            public:
                FunctionCompilationContext(){}
                bytes& text(){
                    return _text;
                }
                const bytes& text() const{
                    return _text;
                }
                std::uint64_t constant(std::uint32_t v){
                    std::uint64_t id = id_list.allocate();
                    values.try_emplace(id,Value::construct<Value::CST>(v));
                    return id;
                }
                void done(std::uint64_t id){
                    vals_t::node_type node{values.extract(id)};
                    switch(node.mapped().kind()){
                        case Value::STACK:
                            stack.deallocate(node.mapped().get<Value::STACK>());
                            break;
                        case Value::REG:
                            used_regs.erase(node.mapped().get<Value::REG>());
                            break;
                        case Value::CST:
                            break;
                    }
                    id_list.deallocate(id);
                }
                x86::Reg store(std::uint64_t v){
                    Value& value = values.at(v);
                    switch(value.kind()){
                        case Value::STACK:
                            return store_v<Value::STACK>(value);
                        case Value::REG:
                            return store_v<Value::REG>(value);
                        case Value::CST:
                            return store_v<Value::CST>(value);
                    }
                }
                void store_d(std::uint64_t v,x86::Reg reg){
                    Value& value = values.at(v);
                    reg_evict(reg);
                    switch(value.kind()){
                        case Value::STACK:
                            return store_v_d<Value::STACK>(value,reg);
                        case Value::REG:
                            return store_v_d<Value::REG>(value,reg);
                        case Value::CST:
                            return store_v_d<Value::CST>(value,reg);
                    }
                }
                template<typename Ins,Value::Kind dstk,Value::Kind srck>
                void insd(Value::type_of<dstk> dst,Value::type_of<srck> src){
                    if constexpr(dstk == Value::STACK){
                        if constexpr(srck == Value::STACK){
                            static_assert(false,"Stack-to-stack move requires an intermediate register");
                        }else if constexpr(srck == Value::REG){
                            Ins::rm_r(_text,x86::width::W32,x86::DisplacementRM<x86::width::W32>(x86::reg::BP),4*dst,src);
                        }else if constexpr(srck == Value::CST){
                            Ins::template rm_imm<x86::width::W32>(_text,x86::DisplacementRM<x86::width::W32>(x86::reg::BP),src,4*dst);
                        }else{
                            static_assert(false,"What type is src?");
                        }
                    }else if constexpr(dstk == Value::REG){
                        if constexpr(srck == Value::STACK){
                            Ins::r_rm(_text,x86::width::W32,dst,x86::DisplacementRM<x86::width::W32>(x86::reg::BP),4*src);
                        }else if constexpr(srck == Value::REG){
                            Ins::rm_r(_text,x86::width::W32,x86::RM(dst),src);
                        }else if constexpr(srck == Value::CST){
                            Ins::template rm_imm<x86::width::W32>(_text,x86::RM(dst),src);
                        }else{
                            static_assert(false,"What type is src?");
                        }
                    }else if constexpr(dstk == Value::CST){
                        static_assert(false,"Cannot modify constant");
                    }else{
                        static_assert(false,"What type is dst?");
                    }
                }
                template<typename Ins,Value::Kind srck>
                void insvd(std::uint64_t dsti,Value::type_of<srck> src){
                    Value& dst = values.at(dsti);
                    if constexpr(std::is_same_v<Ins,x86::encode::mov> && srck == Value::CST){
                        dst.set<Value::CST>(src);
                    }else switch(dst.kind()){
                        case Value::STACK: {
                            Value::type_of<Value::STACK> dstoff = dst.get<Value::STACK>();
                            if constexpr(srck == Value::STACK){
                                insd<Ins,Value::REG,srck>(store_v<Value::STACK>(dst),src);
                            }else{
                                insd<Ins,Value::STACK,srck>(dstoff,src);
                            }
                            break;
                        }
                        case Value::REG:
                            insd<Ins,Value::REG,srck>(dst.get<Value::REG>(),src);
                            break;
                        case Value::CST:
                            insd<Ins,Value::REG,srck>(store_v<Value::CST>(dst),src);
                            break;
                    }
                }
                template<typename Ins,Value::Kind dstk>
                void insdv(Value::type_of<dstk> dst,std::uint64_t srci){
                    static_assert(dstk != Value::CST,"Cannot modify constant");
                    Value& src = values.at(srci);
                    switch(src.kind()){
                        case Value::STACK:
                            Value::type_of<Value::STACK> srcoff = src.get<Value::STACK>();
                            if constexpr(dstk == Value::STACK){
                                insd<Ins,dstk,Value::REG>(dst,store_v<Value::STACK>(src));
                            }else{
                                insd<Ins,dstk,Value::STACK>(dst,src.get<Value::STACK>());
                            }
                            break;
                        case Value::REG:
                            insd<Ins,dstk,Value::REG>(dst,src.get<Value::REG>());
                            break;
                        case Value::CST:
                            insd<Ins,dstk,Value::CST>(dst,src.get<Value::CST>());
                            break;
                    }
                }
                template<typename Ins>
                void ins(std::uint64_t dsti,std::uint64_t srci){
                    Value& src = values.at(srci);
                    switch(src.kind()){
                        case Value::STACK:
                            insvd<Ins,Value::STACK>(dsti,src.get<Value::STACK>());
                            break;
                        case Value::REG:
                            insvd<Ins,Value::REG>(dsti,src.get<Value::REG>());
                            break;
                        case Value::CST:
                            insvd<Ins,Value::CST>(dsti,src.get<Value::CST>());
                            break;
                    }
                }
                std::uint64_t max_stack_size() const{
                    return stack.max_size();
                }
        };
        static std::uint64_t compile_node(const ASTNode& nd,FunctionCompilationContext& fcc){
            switch(nd.type()){
                case 0:{ // ret
                    std::uint64_t value{compile_node(nd.getc(0),fcc)};
                    fcc.store_d(value,x86::reg::A);
                    x86::encode::leave(fcc.text());
                    return std::numeric_limits<std::uint64_t>::max();
                }
                case 1:{ // sub
                    std::uint64_t lhv{compile_node(nd.getc(0),fcc)};
                    std::uint64_t rhv{compile_node(nd.getc(1),fcc)};
                    fcc.ins<x86::encode::sub>(lhv,rhv);
                    fcc.done(rhv);
                    return lhv;
                }
                case 2:{ // lit
                    return fcc.constant(nd.getp(0));
                }
                default:
                    throw 3;
            }
        }
    }
    void targets::Defaultx64::compile(const Function& fn,Text& t) const{
        x64::FunctionCompilationContext fcc;
        compile_node(fn.ast(),fcc);
        x86::encode::sub::rm_imm<x86::width::W32>(t.text(),x86::RM(x86::reg::BP),4*fcc.max_stack_size());
        t.text().append(fcc.text());
    }
}

#include<bbe/targets/x64.hpp>
#include<cppp/polyfill/pack-indexing.hpp>
#include<assembly/instruction.hpp>
#include<cppp/freelist.hpp>
#include<type_traits>
#include<stdexcept>
#include<optional>
#include<variant>
#include<cstdio>
#include<map>
namespace bbe::targets::x64::impl{
    using asm_generic::operator ""_b;
    using Reg = std::byte; //TODO: Rich class with REX support (AH, R8, ...)
    class FunctionCompilationContext{
        bytes _text;
        constexpr static std::array<Reg,3> GENERIC_REGS{x86::reg::A,x86::reg::C,x86::reg::D};
        class Value{
            public:
                enum Kind : std::uint8_t{
                    STACK = 0, REG = 1, CST = 2
                };
                template<Kind k>
                using type_of = cppp::compat::index_pack<
                    static_cast<std::size_t>(k),
                    std::uint32_t,
                    Reg,
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
                type_of<k> get() const{
                    return std::get<static_cast<std::size_t>(k)>(_data);
                }
                void set_rm(x86::Instruction& ins){
                    switch(kind()){
                        case STACK:
                            ins.mod_rm(0b10_b,0b101_b);
                            ins.displacement(-get<STACK>());
                            break;
                        case REG:
                            ins.mod_rm(0b11_b,get<REG>());
                            break;
                        case CST:
                            throw std::logic_error("Value::set_rm: Not R/M");
                    }
                }
                void set_r(x86::Instruction& ins){
                    if(kind()==REG){
                        ins.rm_reg(get<REG>());
                    }else{
                        throw std::logic_error("Value::set_r: Not R");
                    }
                }
                template<Kind k>
                static Value construct(type_of<k> value){
                    return data_t(std::in_place_index<static_cast<std::size_t>(k)>,value);
                }
                Kind kind() const{
                    return static_cast<Kind>(_data.index());
                }
                template<Kind k>
                void set(type_of<k> value){
                    _data.emplace<static_cast<std::size_t>(k)>(value);
                }
        };
        cppp::freelist<std::uint64_t> id_list;
        cppp::freelist<std::uint64_t> stack;
        using vals_t = std::map<std::uint64_t,Value>;
        vals_t values;
        struct reg_less{
            constexpr static bool operator()(Reg lhs,Reg rhs){
                return static_cast<std::uint8_t>(lhs) < static_cast<std::uint8_t>(rhs);
            }
        };
        std::map<Reg,Value*,reg_less> used_regs;
        void reg_evict(Reg r){
            auto node{used_regs.extract(r)};
            if(node){
                node.mapped()->set<Value::STACK>(4*stack.allocate());
            }
        }
        Reg free_reg(){
            for(const Reg r : GENERIC_REGS){
                if(!used_regs.contains(r)){
                    return r;
                }
            }
            reg_evict(GENERIC_REGS.front());
            return GENERIC_REGS.front();
        }
        void reg_set(Reg d,std::uint32_t c){
            x86::Instruction mov;
            x86::encode::mov::rm_imm(mov);
            mov.immediate(c);
            mov.mod_rm(0b11_b,d);
            mov.encode(_text);
        }
        void load_constant_to_reg(Value& v,Reg dr){
            reg_set(dr,v.get<Value::CST>());
            v.set<Value::REG>(dr);
        }
        void load_stack_to_reg(Value& v,Reg dr){
            x86::Instruction mov;
            x86::encode::mov::r_rm(mov);
            mov.rm_reg(dr);
            mov.mod_rm(0b10_b,0b011_b); // [B+disp32]
            mov.displacement(-v.get<Value::STACK>());
            mov.encode(_text);
            v.set<Value::REG>(dr);
        }
        void load_to_reg(Value& v){
            switch(v.kind()){
                case Value::STACK: {
                    Reg dr{free_reg()};
                    load_stack_to_reg(v,dr);
                    used_regs.try_emplace(dr,&v);
                    break;
                }
                case Value::REG:
                    break;
                case Value::CST: {
                    Reg dr{free_reg()};
                    load_constant_to_reg(v,dr);
                    used_regs.try_emplace(dr,&v);
                    break;
                }
            }
        }
        void load_and_set_rm(x86::Instruction& ins,Value& v){
            if(v.kind()==Value::CST){
                Reg dr{free_reg()};
                load_constant_to_reg(v,dr);
                used_regs.try_emplace(dr,&v);
            }
            v.set_rm(ins);
        }
        void load_and_set_r(x86::Instruction& ins,Value& v){
            load_to_reg(v);
            v.set_r(ins);
        }
        void reseat_reg(Reg r,Value& nv){
            reg_evict(r);
            nv.set<Value::REG>(r);
            used_regs.try_emplace(r,&nv);
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
            bool is_constant(std::uint64_t x,std::uint32_t v){
                return values.at(x).kind() == Value::CST && values.at(x).get<Value::CST>() == v;
            }
            void done(std::uint64_t id){
                vals_t::node_type node{values.extract(id)};
                switch(node.mapped().kind()){
                    case Value::STACK:
                        stack.deallocate(node.mapped().get<Value::STACK>()/4);
                        break;
                    case Value::REG:
                        used_regs.erase(node.mapped().get<Value::REG>());
                        break;
                    case Value::CST:
                        break;
                }
                id_list.deallocate(id);
            }
            void write_to_reg(std::uint64_t v,Reg dr){
                Value& value = values.at(v);
                if(value.kind() != Value::REG || value.get<Value::REG>() != dr){
                    reseat_reg(dr,value);
                }
            }
            Reg write_to_free_reg(std::uint64_t v){
                Value& value = values.at(v);
                if(value.kind() != Value::REG){
                    reseat_reg(free_reg(),value);
                }
                return value.get<Value::REG>();
            }
            template<typename Ins,typename Imm>
            void ins_imm(std::uint64_t rm,Imm imm){
                x86::Instruction ins;
                Ins::rm_imm(ins);
                load_and_set_rm(ins,values.at(rm));
                ins.immediate(imm);
                ins.encode(_text);
            }
            template<typename Ins>
            void ins(std::uint64_t dst,std::uint64_t src){
                x86::Instruction ins;
                Value& dstv = values.at(dst);
                Value& srcv = values.at(src);
                if(dstv.kind() == Value::STACK){
                    Ins::rm_r(ins);
                    dstv.set_rm(ins);
                    load_and_set_r(ins,srcv);
                }else{
                    Ins::r_rm(ins);
                    load_and_set_r(ins,dstv);
                    load_and_set_rm(ins,srcv);
                }
                ins.encode(_text);
            }
            std::uint64_t max_stack_size() const{
                return stack.max_size();
            }
    };
    static std::uint64_t compile_node(const ASTNode& nd,FunctionCompilationContext& fcc){
        switch(nd.type()){
            case 0:{ // ret
                std::uint64_t value{compile_node(nd.getc(0),fcc)};
                fcc.write_to_reg(value,x86::reg::A);
                x86::encode::leave(fcc.text());
                fcc.done(value);
                return std::numeric_limits<std::uint64_t>::max();
            }
            case 1:{ // sub
                std::uint64_t lhv{compile_node(nd.getc(0),fcc)};
                std::uint64_t rhv{compile_node(nd.getc(1),fcc)};
                if(!fcc.is_constant(rhv,0)){
                    fcc.ins<x86::encode::sub>(lhv,rhv);
                }
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
    void X64Compiler::compile(const Function& fn,Text& t) const{
        using asm_generic::operator ""_b;
        FunctionCompilationContext fcc;
        compile_node(fn.ast(),fcc);
        x86::Instruction ins;
        x86::encode::push::r_c(ins,x86::reg::BP);
        ins.encode(t.text());
        if(std::uint64_t ss=fcc.max_stack_size()){
            ins.reset();
            x86::encode::sub::rm_imm(ins);
            ins.mod_rm(0b11_b,x86::reg::BP);
            ins.displacement(std::uint32_t(4*ss));
            ins.encode(t.text());
        }
        t.text().append(fcc.text());
    }
}

#include<iostream>
#include<bbe/targets/x64.hpp>
#include<cppp/polyfill/pack-indexing.hpp>
#include<assembly/instruction.hpp>
#include<cppp/freelist.hpp>
#include<cppp/string.hpp> // for symbol imports
#include<type_traits>
#include<stdexcept>
#include<optional>
#include<variant>
#include<limits>
#include<map>
namespace bbe::targets::x64::impl{
    using asm_generic::operator ""_b;
    using Reg = std::byte; //TODO: Rich class with REX support (AH, R8, ...)
    class Value{
        public:
            enum Kind : std::uint8_t{
                STACK = 0, REG = 1, CST = 2, SYM = 3
            };
            template<Kind k>
            using type_of = cppp::compat::index_pack<
                static_cast<std::size_t>(k),
                std::uint32_t,
                Reg,
                std::uint64_t,
                std::uint64_t
            >;
        private:
            template<Kind ...k>
            struct _variant_of_these{
                using type = std::variant<type_of<k>...>;
            };
            using data_t = _variant_of_these<Kind::STACK,Kind::REG,Kind::CST,Kind::SYM>::type;
            data_t _data;
            x86::width _width;
            Value(data_t d,x86::width width) : _data(std::move(d)), _width(width){}
            void set_rm(x86::Instruction& ins) const{
                switch(kind()){
                    case STACK:
                        ins.mod_rm(0b10_b,0b101_b);
                        ins.displacement(-get<STACK>());
                        break;
                    case REG:
                        ins.mod_rm(0b11_b,get<REG>());
                        break;
                    case CST:
                    case SYM:
                        throw std::logic_error("Value::set_rm: Not R/M");
                }
            }
            void set_r(x86::Instruction& ins) const{
                if(kind()==REG){
                    ins.rm_reg(get<REG>());
                }else{
                    throw std::logic_error("Value::set_r: Not R");
                }
            }
            static void encode_instruction_dst(x86::Instruction& ins,const Value& v){
                if(ins.encoding().modrm_is_dst()){
                    v.set_rm(ins);
                }else{
                    v.set_r(ins);
                }
            }
            static void encode_instruction_src(x86::Instruction& ins,const Value& v){
                if(ins.encoding().has_immediate()){
                    if(v.get<CST>()>static_cast<std::uint64_t>(1<<31)){
                        throw std::logic_error("Cannot load immediate value larger than 31 bits");
                    }
                    // If immediate is narrower than 32-bit, the higher bits will get automatically ignored.
                    ins.immediate(static_cast<std::uint32_t>(v.get<CST>()));
                }else{
                    v.set_r(ins);
                }
            }
        public:
            static void encode_instruction(x86::Instruction& ins,const Value& dst,const Value& src){
                encode_instruction_dst(ins,dst);
                encode_instruction_src(ins,src);
                if(ins.encoding().dst_size_as_width()){
                    ins.set_width(dst._width);
                }else{
                    ins.set_width(src._width);
                }
            }
            template<Kind k>
            type_of<k> get() const{
                return std::get<static_cast<std::size_t>(k)>(_data);
            }
            template<Kind k>
            static Value construct(type_of<k> value,x86::width width){
                return {data_t(std::in_place_index<static_cast<std::size_t>(k)>,value),width};
            }
            x86::width width() const{
                return _width;
            }
            Kind kind() const{
                return static_cast<Kind>(_data.index());
            }
            template<Kind k>
            void set(type_of<k> value){
                _data.emplace<static_cast<std::size_t>(k)>(value);
            }
    };
    class FunctionCompilationContext{
        bytes _text;
        std::vector<Relocation> rels;
        constexpr static std::array<Reg,3> GENERIC_REGS{x86::reg::A,x86::reg::C,x86::reg::D};
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
        void load_constant_to_reg(Value& v,Reg dr){
            x86::Instruction mov{x86::encode::mov::rm_imm};
            if(v.width() == x86::width::W64){
                throw std::logic_error("64-bit constants not yet supported");
            }
            mov.immediate(static_cast<std::uint32_t>(v.get<Value::CST>()));
            mov.set_width(x86::width::W32);
            mov.mod_rm(0b11_b,dr);
            mov.encode(_text);
            v.set<Value::REG>(dr);
        }
        void load_sym_to_reg(Value& v,Reg dr){
            x86::Instruction mov{x86::encode::mov::r_rm};
            mov.set_width(v.width());
            mov.mod_rm(0b00_b,0b101_b); // rip + disp32
            mov.rm_reg(dr);
            mov.displacement(std::uint32_t(0));
            rels.emplace_back(mov.encode_and_return_disp(_text),v.get<Value::SYM>());
            rels.back().isize = _text.size()-rels.back().offset;
            v.set<Value::REG>(dr);
        }
        void load_stack_to_reg(Value& v,Reg dr){
            x86::Instruction mov{x86::encode::mov::r_rm};
            mov.rm_reg(dr);
            mov.set_width(x86::width::W32);
            mov.mod_rm(0b10_b,0b011_b); // [BP+disp32]
            mov.displacement(-v.get<Value::STACK>());
            mov.encode(_text);
            v.set<Value::REG>(dr);
        }
        void load_to_reg(Value& v){
            if(v.kind()==Value::REG)return;
            Reg dr{free_reg()};
            switch(v.kind()){
                case Value::STACK:
                    load_stack_to_reg(v,dr);
                    break;
                case Value::CST: 
                    load_constant_to_reg(v,dr);
                    break;
                case Value::SYM:
                    load_sym_to_reg(v,dr);
                    break;
                default: std::unreachable();
            }
            used_regs.try_emplace(dr,&v);
        }
        void load_to_rm(Value& v){
            if(v.kind()!=Value::STACK){
                load_to_reg(v);
            }
        }
        void load_sym_to_rm(Value& v){
            if(v.kind()==Value::SYM){
                Reg dr{free_reg()};
                load_sym_to_reg(v,dr);
                used_regs.try_emplace(dr,&v);
            }
        }
        void reseat_reg(Reg r,Value& nv){
            reg_evict(r);
            nv.set<Value::REG>(r);
            used_regs.try_emplace(r,&nv);
        }
        public:
            using value_handle = std::uint64_t;
            bytes& text(){
                return _text;
            }
            const bytes& text() const{
                return _text;
            }
            template<x86::width w>
            value_handle constant(x86::wv<w> v){
                value_handle id = id_list.allocate();
                values.try_emplace(id,Value::construct<Value::CST>(v,w));
                return id;
            }
            value_handle symbol(std::uint64_t v,x86::width w){
                value_handle id = id_list.allocate();
                values.try_emplace(id,Value::construct<Value::SYM>(v,w));
                return id;
            }
            bool is_constant(value_handle x,std::uint64_t v){
                return values.at(x).kind() == Value::CST && values.at(x).get<Value::CST>() == v;
            }
            void done(value_handle id){
                vals_t::node_type node{values.extract(id)};
                switch(node.mapped().kind()){
                    case Value::STACK:
                        stack.deallocate(node.mapped().get<Value::STACK>()/4);
                        break;
                    case Value::REG:
                        used_regs.erase(node.mapped().get<Value::REG>());
                        break;
                    case Value::CST:
                    case Value::SYM:
                        break;
                }
                id_list.deallocate(id);
            }
            void write_to_reg(value_handle v,Reg dr){
                Value& value = values.at(v);
                if(value.kind() != Value::REG || value.get<Value::REG>() != dr){
                    reseat_reg(dr,value);
                }
            }
            Reg write_to_free_reg(value_handle v){
                Value& value = values.at(v);
                if(value.kind() != Value::REG){
                    reseat_reg(free_reg(),value);
                }
                return value.get<Value::REG>();
            }
            template<typename Ins>
            void encode(value_handle dst,value_handle src){
                Value& dstv = values.at(dst);
                Value& srcv = values.at(src);
                x86::Instruction ins;
                if constexpr(requires{
                    Ins::rm_imm;
                }){
                    if(srcv.kind()==Value::CST){
                        ins.reset(Ins::rm_imm);
                        load_to_rm(dstv);
                        goto end;
                    }else if(srcv.kind()==Value::SYM){
                        ins.reset(Ins::rm_imm);
                        load_sym_to_rm(dstv);
                        goto end;
                    }
                }
                if constexpr(requires{
                    Ins::rm_r;
                }){
                    ins.reset(Ins::rm_r);
                    load_to_rm(dstv);
                    load_to_reg(srcv);
                    goto end;
                }
                if constexpr(requires{
                    Ins::r_rm;
                }){
                    ins.reset(Ins::r_rm);
                    load_to_reg(dstv);
                    load_to_rm(srcv);
                    goto end;
                }
                throw std::logic_error("Can't find suitable encoding for instruction");
                end:
                Value::encode_instruction(ins,dstv,srcv);
                ins.encode(_text);
            }
            value_handle max_stack_size() const{
                return stack.max_size();
            }
            const std::vector<Relocation>& relocations() const{
                return rels;
            }
    };
    static FunctionCompilationContext::value_handle compile_node(const ASTNode& nd,FunctionCompilationContext& fcc){
        using value_handle = FunctionCompilationContext::value_handle;
        switch(nd.type()){
            case 0: // u32
                return fcc.constant<x86::width::W32>(nd.getp(0));
            case 1: // u64
                return fcc.constant<x86::width::W64>(nd.getp(0));
            case 2:{ // add
                value_handle lhv{compile_node(nd.getc(0),fcc)};
                value_handle rhv{compile_node(nd.getc(1),fcc)};
                if(!fcc.is_constant(rhv,0)){
                    fcc.encode<x86::encode::add>(lhv,rhv);
                }
                fcc.done(rhv);
                return lhv;
            }
            case 3:{ // sub
                value_handle lhv{compile_node(nd.getc(0),fcc)};
                value_handle rhv{compile_node(nd.getc(1),fcc)};
                if(!fcc.is_constant(rhv,0)){
                    fcc.encode<x86::encode::sub>(lhv,rhv);
                }
                fcc.done(rhv);
                return lhv;
            }
            case 5:{ // ret
                value_handle value{compile_node(nd.getc(0),fcc)};
                fcc.write_to_reg(value,x86::reg::A);
                fcc.done(value);
                x86::encode::pop::r64(fcc.text(),x86::reg::BP);
                x86::encode::ret::near(fcc.text());
                return std::numeric_limits<value_handle>::max();
            }
            case 100: // sym32
                return fcc.symbol(nd.getp(0),x86::width::W32);
            case 101: // sym64
                return fcc.symbol(nd.getp(0),x86::width::W64);
            default:
                throw 3;
        }
    }
    void X64Program::compile(const Function& fn){
        using asm_generic::operator ""_b;
        FunctionCompilationContext fcc;
        compile_node(fn.ast(),fcc);
        x86::encode::push::r64(_text,x86::reg::BP);
        if(std::uint64_t ss=fcc.max_stack_size()){
            x86::Instruction ins{x86::encode::sub::rm_imm};
            ins.mod_rm(0b11_b,x86::reg::BP);
            ins.set_width(x86::width::W64);
            ins.displacement(std::uint32_t(4*ss));
            ins.encode(_text);
        }
        for(Relocation rel : fcc.relocations()){
            rel.offset += _text.size();
            _relocations.emplace_back(rel);
        }
        _text.append(fcc.text());
    }
}

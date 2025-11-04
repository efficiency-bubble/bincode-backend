#include<bbe/targets/x64-direct.hpp>
#include<cppp/polyfill/pack-indexing.hpp>
#include<assembly/instruction.hpp>
#include<cppp/freelist.hpp>
#include<cppp/string.hpp> // for symbol imports
#include<cppp/ptr.hpp>
#include<type_traits>
#include<stdexcept>
#include<optional>
#include<variant>
#include<limits>
#include<map>
namespace bbe::targets::x64d::impl{
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
            template<Kind>
            struct value_kind_t{};
            template<Kind ...k>
            struct _variant_of_these{
                using type = std::variant<type_of<k>...>;
            };
            using data_t = _variant_of_these<Kind::STACK,Kind::REG,Kind::CST,Kind::SYM>::type;
            data_t _data;
            x86::width _width;
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
        public:
            void encode_instruction_dst(x86::Instruction& ins) const{
                if(ins.encoding().dst_size_as_width()){
                    ins.set_width(_width);
                }
                if(ins.encoding().modrm_is_dst()){
                    set_rm(ins);
                }else{
                    set_r(ins);
                }
            }
            void encode_instruction_src(x86::Instruction& ins) const{
                if(!ins.encoding().dst_size_as_width()){
                    ins.set_width(_width);
                }
                if(ins.encoding().has_immediate()){
                    if(get<CST>()>static_cast<std::uint64_t>(1<<31)){
                        throw std::logic_error("Cannot load immediate value larger than 31 bits");
                    }
                    // If immediate is narrower than 32-bit, the higher bits will get automatically ignored.
                    ins.immediate(static_cast<std::uint32_t>(get<CST>()));
                }else if(ins.encoding().has_modrm()&&!ins.encoding().modrm_is_dst()){
                    set_rm(ins);
                }else{
                    set_r(ins);
                }
            }
            template<Kind k>
            constexpr static value_kind_t<k> value_kind{};
            Value() = default;
            Value(const Value&) = delete;
            Value(Value&&) = delete;
            Value& operator=(const Value&) = delete;
            Value& operator=(Value&&) = delete;
            template<Kind k>
            type_of<k> get() const{
                return std::get<static_cast<std::size_t>(k)>(_data);
            }
            template<Kind k>
            Value(value_kind_t<k>,type_of<k> value,x86::width width)
            : _data(std::in_place_index<static_cast<std::size_t>(k)>,value), _width(width){}
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
        public:
            using value_handle = std::uint64_t;
        private:
            bytes _text;
            std::vector<Relocation> rels;
            constexpr static std::array<Reg,5> GENERIC_REGS{x86::reg::A,x86::reg::C,x86::reg::D,x86::reg::SI,x86::reg::DI};
            cppp::freelist<std::uint64_t> id_list;
            cppp::freelist<value_handle> stack;
            using vals_t = std::map<value_handle,std::pair<Value,std::uint32_t>>;
            vals_t values;
            struct reg_less{
                constexpr static bool operator()(Reg lhs,Reg rhs){
                    return static_cast<std::uint8_t>(lhs) < static_cast<std::uint8_t>(rhs);
                }
            };
            std::map<Reg,value_handle> used_regs;
            void reg_into_stack(value_handle vh){
                Value& v = values.at(vh).first;
                x86::Instruction mov{x86::encode::mov::rm_r};
                v.encode_instruction_src(mov);
                mov.mod_rm(0b10_b,0b011_b); // [BP+disp32]
                std::uint32_t soff = static_cast<std::uint32_t>(4*stack.allocate());
                mov.displacement(soff);
                v.set<Value::STACK>(soff);
            }
            void reg_evict(Reg r){
                if(auto it=used_regs.find(r);it!=used_regs.end()){
                    Reg sr = it->first;
                    for(const Reg r : GENERIC_REGS){
                        if(!used_regs.contains(r)){
                            Value& v = values.at(it->second).first;
                            
                            x86::Instruction mov{x86::encode::mov::rm_r};
                            mov.set_width(v.width());
                            v.encode_instruction_src(mov);
                            mov.mod_rm(0b11_b,r);
                            v.set<Value::REG>(r);
                            used_regs.try_emplace(r,it->second);
                            mov.encode(_text);
                            
                            used_regs.erase(sr);
                            return;
                        }
                    }
                    reg_into_stack(it->second);
                    used_regs.erase(it);
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
            void load_to_reg(value_handle vh,Reg dr){
                Value& v = values.at(vh).first;
                bool cst = v.kind() == Value::CST;
                x86::Instruction mov{cst?x86::encode::mov::rm_imm:x86::encode::mov::r_rm};
                mov.set_width(v.width());
                if(cst){
                    mov.mod_rm(0b11_b,dr);
                }else{
                    mov.rm_reg(dr);
                }
                if(v.kind() == Value::SYM){
                    mov.mod_rm(0b00_b,0b101_b); // rip + disp32
                    mov.displacement(std::uint32_t(0));
                    rels.emplace_back(mov.encode_and_return_disp(_text),v.get<Value::SYM>());
                    rels.back().isize = _text.size()-rels.back().offset;
                }else{
                    v.encode_instruction_src(mov);
                    mov.encode(_text);
                }
                v.set<Value::REG>(dr);
                used_regs.try_emplace(dr,vh);
            }
            void load_to_free_reg(value_handle vh){
                if(values.at(vh).first.kind()==Value::REG)return;
                load_to_reg(vh,free_reg());
            }
            void load_sym_to_rm(value_handle vh){
                Value& v = values.at(vh).first;
                if(v.kind()==Value::SYM){
                    load_to_free_reg(vh);
                }
            }
            void reseat_reg(Reg r,value_handle vh){
                reg_evict(r);

                load_to_rm(vh);
                Value& nv = values.at(vh).first;
                
                x86::Instruction mov{x86::encode::mov::r_rm};
                mov.set_width(nv.width());
                nv.encode_instruction_src(mov);
                mov.rm_reg(r);
                mov.encode(_text);
                
                used_regs.erase(nv.get<Value::REG>());
                nv.set<Value::REG>(r);
                used_regs.try_emplace(r,vh);
            }
        public:
            void debug() const{
                printf("debug values\n");
                for(const auto& [vh,_] : values){
                    printf("v %lu\n",vh);
                }
                printf("debug register status\n");
                for(const auto& [r,v] : used_regs){
                    printf("R %d > v %lu\n",int(r),v);
                }
                printf("debug end\n");
            }
            void reg_evacuate(){
                for(const auto& [_,v] : used_regs){
                    reg_into_stack(v);
                }
                used_regs.clear();
            }
            const Value& operator[](std::size_t i) const{
                return values.at(i).first;
            }
            void load_to_rm(value_handle vh){
                Value& v = values.at(vh).first;
                if(v.kind()!=Value::STACK){
                    load_to_free_reg(vh);
                }
            }
            bytes& text(){
                return _text;
            }
            const bytes& text() const{
                return _text;
            }
            template<x86::width w>
            value_handle constant(x86::wv<w> v){
                value_handle id = id_list.allocate();
                values.try_emplace(id,std::piecewise_construct,std::forward_as_tuple(Value::value_kind<Value::CST>,v,w),std::forward_as_tuple(1));
                return id;
            }
            value_handle symbol(std::uint64_t v,x86::width w){
                value_handle id = id_list.allocate();
                values.try_emplace(id,std::piecewise_construct,std::forward_as_tuple(Value::value_kind<Value::SYM>,v,w),std::forward_as_tuple(1));
                return id;
            }
            bool is_constant(value_handle x,std::uint64_t v){
                return values.at(x).first.kind() == Value::CST && values.at(x).first.get<Value::CST>() == v;
            }
            void done(value_handle id){
                vals_t::iterator m{values.find(id)};
                if(!--m->second.second){
                    switch(m->second.first.kind()){
                        case Value::STACK:
                        stack.deallocate(m->second.first.get<Value::STACK>()/4);
                        break;
                        case Value::REG:
                        used_regs.erase(m->second.first.get<Value::REG>());
                        break;
                        case Value::CST:
                        case Value::SYM:
                        break;
                    }
                    id_list.deallocate(id);
                    values.erase(m);
                }
            }
            value_handle value_in_reg(Reg r,x86::width w){
                if(auto it=used_regs.find(r);it!=used_regs.end()){
                    ++values.at(it->second).second;
                    return it->second;
                }
                value_handle id = id_list.allocate();
                values.try_emplace(id,std::piecewise_construct,std::forward_as_tuple(Value::value_kind<Value::REG>,r,w),std::forward_as_tuple(1));
                used_regs.try_emplace(r,id);
                return id;
            }
            void write_to_reg(value_handle v,Reg dr){
                Value& value = values.at(v).first;
                if(value.kind() != Value::REG || value.get<Value::REG>() != dr){
                    reseat_reg(dr,v);
                }
            }
            Reg write_to_free_reg(value_handle v){
                Value& value = values.at(v).first;
                if(value.kind() != Value::REG){
                    reseat_reg(free_reg(),v);
                }
                return value.get<Value::REG>();
            }
            template<typename Ins>
            void encode(value_handle dst,value_handle src){
                Value& dstv = values.at(dst).first;
                Value& srcv = values.at(src).first;
                x86::Instruction ins;
                if constexpr(requires{
                    Ins::rm_imm;
                }){
                    if(srcv.kind()==Value::CST){
                        ins.reset(Ins::rm_imm);
                        load_to_rm(dst);
                        goto end;
                    }else if(srcv.kind()==Value::SYM){
                        ins.reset(Ins::rm_imm);
                        load_sym_to_rm(src);
                        goto end;
                    }
                }
                if constexpr(requires{
                    Ins::rm_r;
                }){
                    ins.reset(Ins::rm_r);
                    load_to_rm(dst);
                    load_to_free_reg(src);
                    goto end;
                }
                if constexpr(requires{
                    Ins::r_rm;
                }){
                    ins.reset(Ins::r_rm);
                    load_to_free_reg(dst);
                    load_to_rm(src);
                    goto end;
                }
                throw std::logic_error("Can't find suitable encoding for instruction");
                end:
                dstv.encode_instruction_dst(ins);
                srcv.encode_instruction_src(ins);
                ins.encode(_text);
            }
            value_handle max_stack_size() const{
                return stack.max_size();
            }
            const std::vector<Relocation>& relocations() const{
                return rels;
            }
    };
    static FunctionCompilationContext::value_handle compile_node(const ASTNode& nd,FunctionCompilationContext& fcc,bool& subfns){
        static Reg X86_ABI_ARG_REGS[]{x86::reg::DI,x86::reg::SI,x86::reg::D,x86::reg::C};
        using value_handle = FunctionCompilationContext::value_handle;
        switch(nd.type()){
            case 0: // u32
                return fcc.constant<x86::width::W32>(static_cast<std::uint32_t>(nd.getp()));
            case 1: // u64
                return fcc.constant<x86::width::W64>(nd.getp());
            case 2:{ // add
                value_handle lhv{compile_node(nd.children()[0],fcc,subfns)};
                value_handle rhv{compile_node(nd.children()[1],fcc,subfns)};
                if(!fcc.is_constant(rhv,0)){
                    fcc.encode<x86::encode::add>(lhv,rhv);
                }
                fcc.done(rhv);
                return lhv;
            }
            case 3:{ // sub
                value_handle lhv{compile_node(nd.children()[0],fcc,subfns)};
                value_handle rhv{compile_node(nd.children()[1],fcc,subfns)};
                if(!fcc.is_constant(rhv,0)){
                    fcc.encode<x86::encode::sub>(lhv,rhv);
                }
                fcc.done(rhv);
                return lhv;
            }
            case 5: // arg32
                return fcc.value_in_reg(X86_ABI_ARG_REGS[nd.getp()],x86::width::W32);
            case 6:{ // arg64
                return fcc.value_in_reg(X86_ABI_ARG_REGS[nd.getp()],x86::width::W64);
            }
            case 7:{ // ret
                value_handle value{compile_node(nd.children()[0],fcc,subfns)};
                fcc.write_to_reg(value,x86::reg::A);
                fcc.done(value);
                x86::encode::pop::r64(fcc.text(),x86::reg::BP);
                x86::encode::ret::near(fcc.text());
                return std::numeric_limits<value_handle>::max();
            }
            case 8:{ // callf
                subfns = true;
                value_handle dst = compile_node(nd.children()[0],fcc,subfns);
                for(std::size_t i=1uz;i<nd.children().size();++i){
                    value_handle arg = compile_node(nd.children()[i],fcc,subfns);
                    fcc.write_to_reg(arg,X86_ABI_ARG_REGS[i-1uz]);
                    fcc.done(arg);
                }
                x86::Instruction ins{x86::encode::call::rm};
                fcc.load_to_rm(dst);
                fcc[dst].encode_instruction_dst(ins);
                fcc.reg_evacuate();
                ins.encode(fcc.text());
                fcc.done(dst);
                return fcc.value_in_reg(x86::reg::A,x86::width::W32);
            }
            case 100: // sym32
                return fcc.symbol(nd.getp(),x86::width::W32);
            case 101: // sym64
                return fcc.symbol(nd.getp(),x86::width::W64);
            default:
                throw 3;
        }
    }
    void X64Program::compile(const Function& fn){
        using asm_generic::operator ""_b;
        FunctionCompilationContext fcc;
        bool subfns = false;
        compile_node(fn.ast(),fcc,subfns);
        x86::encode::push::r64(_text,x86::reg::BP);
        if(std::uint64_t ss=fcc.max_stack_size()){
            if(subfns){
                ss = (ss+1)&-std::uint64_t(2);
            }
            x86::Instruction ins{x86::encode::sub::rm_imm};
            ins.mod_rm(0b11_b,x86::reg::BP);
            ins.set_width(x86::width::W64);
            ins.immediate(std::uint32_t(4*ss));
            ins.encode(_text);
        }
        for(Relocation rel : fcc.relocations()){
            rel.offset += _text.size();
            _relocations.emplace_back(rel);
        }
        _text.append(fcc.text());
    }
}

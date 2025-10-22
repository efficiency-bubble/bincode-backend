#include<bbe/targets/ssa.hpp>
#include<unordered_map>
#include<functional>
#include<stdexcept>
#include<compare>
#include<ranges>
namespace bbe::targets::ssa::impl{
    cppp::str Instruction::debug() const{
        cppp::str string{cppp::tou8(std::to_string(dst))};
        string.append(u8" = "sv);
        string.append(stringify_enum(opcode));
        for(std::uint32_t s : src){
            string.push_back(u8' ');
            string.append(cppp::tou8(std::to_string(s)));
        }
        return string;
    }
    std::uint32_t BasicBlock::new_value_for_name(std::uint32_t name){
        std::uint32_t vid = va->next_value++;
        bind_name(name,vid);
        return vid;
    }
    class FunctionCompiler{
        ProcedureIC* ic;
        std::uint32_t name_count = 0;
        std::uint32_t current_block_id;
        std::uint32_t new_name(){
            return name_count++;
        }
        std::uint32_t compile_value(const ASTNode& nd){
            std::uint32_t name = compile_node(nd);
            return current_block().value_of(name);
        }
        BasicBlock& current_block(){
            return ic->blocks()[current_block_id];
        }
        // operation(changesBlockID()) will write to the new block, unlike current_block().operation()
        void operation(Operation opr,std::uint32_t name,std::vector<std::uint32_t>&& argv){
            current_block().operation(opr,name,std::move(argv));
        }
        
        std::uint32_t opnode(Operation opr,const ASTNode& nd){
            std::uint32_t name = new_name();
            std::vector<std::uint32_t> values;
            for(const ASTNode& child : nd.children()){
                values.emplace_back(compile_value(child));
            }
            operation(opr,name,std::move(values));
            return name;
        }
        std::uint32_t compile_node(const ASTNode& nd){
            std::uint32_t name = BasicBlock::NNAME;
            switch(nd.type()){
                case 0: // u32
                    current_block().imm32(name = new_name(),static_cast<std::uint32_t>(nd.getp()));
                    break;
                case 1: // u64
                    current_block().imm64(name = new_name(),nd.getp());
                    break;
                case 2: // pack
                    name = opnode(Operation::PACK,nd);
                    break;
                case 5: // arg32
                case 6: // arg64
                case 22: // argb
                    operation(Operation::LDAR,name = new_name(),{static_cast<std::uint32_t>(nd.getp())});
                    break;
                case 7: { // ret
                    std::uint32_t retv = compile_value(nd.children()[0]);
                    current_block().retf(retv);
                    break;
                }
                case 8: // callf
                    name = opnode(Operation::CALL,nd);
                    break;
                case 9: // call magic
                    operation(Operation::CMAG,name = new_name(),{static_cast<std::uint32_t>(nd.getp()),compile_value(nd.children()[0])});
                    break;
                case 10: // setvar
                    // TODO: support over 100k intermediate results
                    current_block().bind_name(name = static_cast<std::uint32_t>(nd.getp()+100000),compile_value(nd.children()[0]));
                    break;
                case 11: // getvar
                    // TODO: support over 100k intermediate results
                    current_block().bind_name(name = new_name(),current_block().value_of(static_cast<std::uint32_t>(nd.getp()+100000)));
                    break;
                case 20: // bool
                    current_block().immb(name = new_name(),bool(nd.getp()));
                    break;
                case 21: { // fork
                    std::uint32_t cond = compile_value(nd.children()[0]);
                    std::uint32_t lhb = ic->new_block();
                    std::uint32_t rhb = ic->new_block();
                    std::uint32_t continuation = ic->new_block();
                    name = new_name();
                    current_block().r_branch(cond,rhb,lhb);
                    
                    current_block_id = rhb;
                    current_block().r_always(continuation);
                    current_block().bind_name(name,compile_value(nd.children()[1]));
                    
                    current_block_id = lhb;
                    current_block().r_always(continuation);
                    current_block().bind_name(name,compile_value(nd.children()[2]));
                    
                    current_block_id = continuation;
                    break;
                }
                case 64: // compound
                    for(const ASTNode& ch : nd.children()){
                        name = compile_node(ch);
                    }
                    break;
                case 100: // sym32
                case 101: // sym64
                    operation(Operation::LDS,name = new_name(),{static_cast<std::uint32_t>(nd.getp())});
                    break;
                default:
                    throw std::logic_error("SSA compile: unknown node type "s+std::to_string(nd.type()));
            }
            return name;
        }
        public:
            FunctionCompiler(ProcedureIC& c) : ic(&c){}
            void compile_block(std::uint32_t bid,const ASTNode& nd){
                current_block_id = bid;
                compile_value(nd);
            }
    };
    // struct ValTransfer{
    //     std::uint32_t from;
    //     std::uint32_t to;
    // };
    // struct BlockTransfer{
    //     std::uint32_t to;
    //     std::vector<ValTransfer> transfers;
    // };
    // void write_transfers(BasicBlock& blk,const BlockTransfer& tt){
    //     for(const ValTransfer& vt : tt.transfers){
    //         blk.instruction(Operation::MOV,vt.to,std::vector<std::uint32_t>{vt.from});
    //     }
    // }
    // void add_transfers(ValueAllocation& alloc,std::deque<BasicBlock>& blocks){
    //     std::size_t nblk = blocks.size();
    //     std::vector<BlockTransfer> transfers;
    //     for(auto& block : blocks){
    //         if(block.retcond()==block.NCOND){
    //             if(!block.retlocs().empty()){
    //                 for(const auto& [n,v] : blocks[block.retlocs().front()].imports()){
    //                     block.instruction(Operation::MOV,v,std::vector<std::uint32_t>{block.nametable().at(n)});
    //                 }
    //             }
    //         }else{
    //             for(std::uint32_t& dst : block.retlocs()){
    //                 if(!blocks[dst].imports().empty()){
    //                     std::uint32_t olddst = std::exchange(dst,nblk+transfers.size());
    //                     std::vector<ValTransfer>& vtt = transfers.emplace_back(olddst).transfers;
    //                     for(const auto& [n,v] : blocks[olddst].imports()){
    //                         vtt.emplace_back(block.nametable().at(n),v);
    //                     }
    //                 }
    //             }
    //         }
    //     }
    //     for(const BlockTransfer& trans : transfers){
    //         BasicBlock& tblk = blocks.emplace_back(alloc);
    //         write_transfers(tblk,trans);
    //         tblk.r_always(trans.to);
    //     }
    // }
    ProcedureIC::ProcedureIC(const Function& fn) : _blocks{alloc}{
        FunctionCompiler fc{*this};
        fc.compile_block(0,fn.ast());
        // add_transfers(alloc,_blocks);
    }
}

import std.io;
import std.string;
fn decode_uleb128(){
    u16 n;
    u32 value = 0;
    while(true){
        u8 b = $[$+n];
        value += (b & 0x7f) << (n * 7);
        $ += 1;
        n += 1;
        if(b & 0x80) return value;
    }
};
struct invleb128{
    u32 value = decode_uleb128() [[export]];
    padding[while($[$] & 0x80 == 0)];
    padding[1];
} [[sealed,format_read("leb128v")]];
fn leb128v(ref invleb128 v){
    return std::string::to_string(v.value);
};
enum NodeType : u8{
    UINT32,UINT64,PACK,COMMA,ARG=5,CALL_BUILTIN=9,SETVAR,GETVAR,BOOL=20,FORK,FOREVER=30,BREAK,UINT32SYM=100,FNSYM=200,
    NTYPE = 255
};
u32 VARIABLE = (1<<32-1);
fn nchld(NodeType t){
    if(t==NodeType::UINT32){
        return 0;
    }else if(t==NodeType::UINT64){
        return 0;
    }else if(t==NodeType::BOOL){
        return 0;
    }else if(t==NodeType::ARG){
        return 0;
    }else if(t==NodeType::GETVAR){
        return 0;
    }else if(t==NodeType::BREAK){
        return 0;
    }else if(t==NodeType::UINT32SYM){
        return 0;
    }else if(t==NodeType::FNSYM){
        return 0;
    }else if(t==NodeType::NTYPE){
        return 0;
    }else if(t==NodeType::SETVAR){
        return 0;
    }else if(t==NodeType::CALL_BUILTIN){
        return 1;
    }else if(t==NodeType::FOREVER){
        return 1;
    }else if(t==NodeType::FORK){
        return 3;
    }else{
        return VARIABLE;
    }
};
fn has_extended_data(NodeType t){
    return t == NodeType::UINT64;
};
struct ASTNode{
    NodeType type;
    invleb128 prim;
    u32 nc = nchld(type) [[export]];
    if(nc==VARIABLE){
        invleb128 _nchld;
        nc = _nchld.value;
    }
    if(has_extended_data(type)){
        u64 e_prim;
    }
    ASTNode children[nc];
};
ASTNode root @ 0;

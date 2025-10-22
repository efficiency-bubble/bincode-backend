#include<assembly/common.hpp>
#include<cppp/bytearray.hpp>
#include<cppp/string.hpp>
#include<bbe/bbe.hpp>
#include<type_traits>
#include<cinttypes>
#include<ranges>
#include<cstdio>
#include<stack>
#define DBGPRINT(pat,...)
// #define DBGPRINT(pat,...) fprintf(stderr,pat __VA_OPT__(,) __VA_ARGS__)
using namespace bbe;
using namespace std::literals;
using asm_generic::operator ""_b;
char8_t read(){
    using it = std::char_traits<char8_t>::int_type;
    it ch = static_cast<it>(std::getchar());
    if(ch == std::char_traits<char8_t>::eof()){
        std::exit(0);
    }
    DBGPRINT("read %d\n",ch);
    return std::char_traits<char8_t>::to_char_type(ch);
}
std::byte bread(){
    return static_cast<std::byte>(read());
}
template<typename T> requires(std::is_integral_v<T> && std::is_unsigned_v<T>)
T ri(){
    T ret;
    std::fread(&ret,sizeof(T),1uz,stdin);
    DBGPRINT("readi %" PRIu64 "\n",std::uint64_t(ret));
    if(std::endian::native == std::endian::little){
        return ret;
    }else{
        return std::byteswap(ret);
    }
}
ASTNode aread(){
    return {ri<std::uint32_t>(),ri<std::uint64_t>()};
}
int main(){
    ProjectEntitiesPool entities;
    freopen(nullptr,"rb",stdin);
    freopen(nullptr,"wb",stdout);
    cppp::bytes buf;
    std::size_t ndata;
    std::stack<ASTNode> holding_area;
    while(true){
        switch(bread()){
            case 0_b: // New function
                buf.appendl<std::uint64_t>(entities.function_pool().emplace(nullptr));
                break;
            case 1_b: // Delete function
                entities.function_pool().pop(static_cast<ProjectEntitiesPool::index_type>(ri<std::uint64_t>()));
                break;
            case 2_b: // Get function root
                buf.appendl<std::uint64_t>(reinterpret_cast<std::uint64_t>(&entities.function_pool()[static_cast<ProjectEntitiesPool::index_type>(ri<std::uint64_t>())].ast()));
                break;
            case 9_b: { // Count children
                ASTNode* naddr = reinterpret_cast<ASTNode*>(ri<std::uint64_t>());
                buf.appendl<std::uint64_t>(naddr->children().size());
                break;
            }
            case 10_b: { // Reset node
                ASTNode* naddr = reinterpret_cast<ASTNode*>(ri<std::uint64_t>());
                *naddr = aread();
                break;
            }
            case 11_b: { // Get child
                ASTNode* naddr = reinterpret_cast<ASTNode*>(ri<std::uint64_t>());
                buf.appendl<std::uint64_t>(reinterpret_cast<std::uint64_t>(&naddr->children()[ri<std::uint64_t>()]));
                break;
            }
            case 12_b: { // Get type
                ASTNode* naddr = reinterpret_cast<ASTNode*>(ri<std::uint64_t>());
                buf.appendl<std::uint32_t>(naddr->type());
                break;
            }
            case 13_b: { // Get primitive
                ASTNode* naddr = reinterpret_cast<ASTNode*>(ri<std::uint64_t>());
                buf.appendl<std::uint64_t>(naddr->getp());
                break;
            }
            case 14_b: { // Set primitive
                ASTNode* naddr = reinterpret_cast<ASTNode*>(ri<std::uint64_t>());
                switch(bread()){
                    case 32_b: // ui32
                        naddr->setp(ri<std::uint64_t>());
                        break;
                }
                break;
            }
            case 15_b: { // Move node into holding area
                ASTNode* naddr = reinterpret_cast<ASTNode*>(ri<std::uint64_t>());
                holding_area.emplace(std::move(*naddr));
                break;
            }
            case 16_b: { // Pop node from holding area
                ASTNode* naddr = reinterpret_cast<ASTNode*>(ri<std::uint64_t>());
                *naddr = std::move(holding_area.top());
                holding_area.pop();
                break;
            }
            case 250_b: { // List entities
                for(const auto& k : entities.function_pool() | std::views::keys){
                    buf.appendl<std::uint64_t>(k);
                }
                break;
            }
        }
        ndata = std::endian::native==std::endian::little?buf.size():std::byteswap(buf.size());
        std::fwrite(&ndata,sizeof(ndata),1uz,stdout);
        std::fwrite(buf.data(),buf.size(),1uz,stdout);
        std::fflush(stdout);
        buf.clear();
    }
    return 0;
}

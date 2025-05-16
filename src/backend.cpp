#include<cppp/bfile.hpp>
#include<cppp/freelist.hpp>
#include<assembly/common.hpp>
#include<cppp/indexed-array-pool.hpp>
#include<bbe/bbe.hpp>
#include<bbe/targets/x64.hpp>
#include<bbe/formats/elf.hpp>
#include<type_traits>
#include<cinttypes>
#include<optional>
#include<cstdio>
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
    return {ri<std::uint64_t>(),ri<std::uint64_t>()};
}
template<typename T> requires(std::is_integral_v<T> && std::is_unsigned_v<T>)
void wi(std::type_identity_t<T> x){
    if(std::endian::native != std::endian::little){
        x = std::byteswap(x);
    }
    DBGPRINT("write %" PRIu64 "\n",std::uint64_t(x));
    std::fwrite(&x,sizeof(T),1uz,stdout);
}
int main(){
    Function main{nullptr,{},ASTNode()};
    freopen(nullptr,"rb",stdin);
    freopen(nullptr,"wb",stdout);
    ASTNode* naddr{nullptr};
    std::uint64_t arg;
    wi<std::uint64_t>(reinterpret_cast<std::uint64_t>(&main.ast()));
    while(true){
        switch(bread()){
            case 0_b: { // Set node
                naddr = reinterpret_cast<ASTNode*>(ri<std::uint64_t>());
                *naddr = aread();
                break;
            }
            case 1_b: // Add child
                naddr = reinterpret_cast<ASTNode*>(ri<std::uint64_t>());
                wi<std::uint64_t>(reinterpret_cast<std::uint64_t>(&naddr->children().emplace_back(aread())));
                std::fflush(stdout);
                break;
            case 2_b: // Delete (in fact, blank) node
                *naddr = {};
                break;
            case 3_b: // Set primitive
                naddr = reinterpret_cast<ASTNode*>(ri<std::uint64_t>());
                arg = ri<std::uint64_t>();
                switch(bread()){
                    case 32_b: // ui32
                        naddr->setp(arg,ri<std::uint64_t>());
                        break;
                }
                break;
            case 40_b: { // Compile
                targets::x64::X64Program prog;
                DBGPRINT("Comp.A");
                prog.compile(main);
                DBGPRINT("Comp.B");
                wi<std::uint64_t>(prog.text().size());
                DBGPRINT("Comp.C");
                formats::elf::Elf elf;
                elf.add_text(prog);
                cppp::bytes elfdata{elf.encode()};
                fwrite(elfdata.data(),elfdata.size(),1uz,stdout);
                DBGPRINT("Comp.D");
                std::fflush(stdout);
                DBGPRINT("Comp.E");
                break;
            }
        }
    }
    return 0;
}

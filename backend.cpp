#include<cppp/bfile.hpp>
#include<assembly/common.hpp>
#include<bbe/bbe.hpp>
#include<type_traits>
#include<cinttypes>
#include<optional>
#include<cstdio>
#define DBGPRINT(pat,...)
// #define DBGPRINT(pat,...) fprintf(stderr,pat __VA_OPT__(,) __VA_ARGS__)
using namespace bbe;
using namespace std::literals;
using asm_generic::operator ""_b;
template<typename T>
class AllocatedArray{
    std::vector<std::optional<T>> arr;
    bbe::impl::Allocator alloc;
    public:
        template<typename ...A>
        std::uint64_t emplace(A&& ...v){
            std::uint64_t indx = alloc.push();
            if(indx > arr.size()){
                throw std::runtime_error("allocator/array size mismatch");
            }
            if(indx == arr.size()){
                arr.emplace_back(std::in_place,std::forward<A>(v)...);
            }else{
                arr[indx].emplace(std::forward<A>(v)...);
            }
            return indx;
        }
        T& operator[](std::uint64_t indx){
            return *arr[indx];
        }
        const T& operator[](std::uint64_t indx) const{
            return *arr[indx];
        }
        void free(std::uint64_t indx){
            alloc.pop(indx);
            arr[indx].reset();
        }
};
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
    Function main{u8"main"s,nullptr,{},ASTNode()};
    freopen(nullptr,"rb",stdin);
    freopen(nullptr,"wb",stdout);
    std::uint64_t naddr;
    std::uint64_t arg;
    Default_AMD64 compiler;
    wi<std::uint64_t>(reinterpret_cast<std::uint64_t>(&main.ast()));
    while(true){
        switch(bread()){
            case 0_b: { // Set node
                naddr = ri<std::uint64_t>();
                *reinterpret_cast<ASTNode*>(naddr) = aread();
                break;
            }
            case 1_b: // Add child
                naddr = ri<std::uint64_t>();
                wi<std::uint64_t>(reinterpret_cast<std::uint64_t>(&reinterpret_cast<ASTNode*>(naddr)->children().emplace_back(aread())));
                std::fflush(stdout);
                break;
            case 2_b: // Delete (in fact, blank) node
                *reinterpret_cast<ASTNode*>(naddr) = {};
                break;
            case 3_b: // Set primitive
                naddr = ri<std::uint64_t>();
                arg = ri<std::uint64_t>();
                switch(bread()){
                    case 32_b: // ui32
                        reinterpret_cast<ASTNode*>(naddr)->setp(arg,ri<std::uint32_t>());
                        break;
                }
                break;
            case 40_b: { // Compile
                Text text;
                DBGPRINT("Comp.A");
                compiler.compile(main,text);
                DBGPRINT("Comp.B");
                wi<std::uint64_t>(text.text().size());
                DBGPRINT("Comp.C");
                fwrite(text.text().data(),text.text().size(),1uz,stdout);
                DBGPRINT("Comp.D");
                std::fflush(stdout);
                DBGPRINT("Comp.E");
                break;
            }
        }
    }
    return 0;
}

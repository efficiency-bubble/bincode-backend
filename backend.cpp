#include<cppp/bfile.hpp>
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
constexpr static std::uint32_t CONTEXT_IDENTIFIER_C = 0;
constexpr static std::uint32_t CONTEXT_VALUE_C = 1;
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
    return {ri<std::uint64_t>(),ri<std::uint32_t>()};
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
    std::optional<ASTNode> root{std::nullopt};
    freopen(nullptr,"rb",stdin);
    freopen(nullptr,"wb",stdout);
    std::uint32_t naddr;
    std::uint32_t arg;
    Compiler env{default_amd64()};
    while(true){
        switch(bread()){
            case 0_b: // Create root
                root = aread();
                wi<std::uint64_t>(reinterpret_cast<std::uint64_t>(&*root));
                std::fflush(stdout);
                break;
            case 1_b: { // Create child
                naddr = ri<std::uint64_t>();
                arg = ri<std::uint64_t>();
                std::fflush(stdout);
                wi<std::uint64_t>(reinterpret_cast<std::uint64_t>(&reinterpret_cast<ASTNode*>(naddr)->emplace(arg,aread())));
                break;
            }
            case 2_b: // Set primitive
                naddr = ri<std::uint64_t>();
                arg = ri<std::uint64_t>();
                switch(bread()){
                    case 32_b: // ui32
                        reinterpret_cast<ASTNode*>(naddr)->setp(arg,ri<std::uint32_t>());
                        break;
                }
                break;
            case 4_b: // Delete node
                break;
            case 12_b: // List autocomplete contexts
                wi<std::uint64_t>(2);
                wi<std::uint32_t>(CONTEXT_IDENTIFIER_C);
                wi<std::uint32_t>(CONTEXT_VALUE_C);
                std::fflush(stdout);
                break;
            case 40_b: { // Compile
                Text text;
                FunctionCompilationContext cc{text};
                DBGPRINT("Comp.A");
                env.compile(*root,cc);
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

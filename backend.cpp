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
using cppp::ptr;
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
ptr<ASTNode> aread(){
    switch(bread()){
        case 0_b: // Builtin
            switch(bread()){
                case 0_b: // Return
                    return ptr<Return>::construct(nullptr);
            }
            break;
        case 1_b: // Operator
            switch(bread()){
                case 0_b: // Sub
                    return ptr<Subi32>::construct(nullptr,nullptr);
            }
            break;
        case 2_b: // Literal
            switch(bread()){
                case 0_b: // Num
                    return ptr<Constanti32>::construct(0);
            }
            break;
    }
    throw std::runtime_error("aread(): unknown sequence");
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
template<typename T> requires(std::is_integral_v<T> && std::is_unsigned_v<T>)
void wi(std::type_identity_t<T> x){
    if(std::endian::native != std::endian::little){
        x = std::byteswap(x);
    }
    DBGPRINT("write %" PRIu64 "\n",std::uint64_t(x));
    std::fwrite(&x,sizeof(T),1uz,stdout);
}
int main(){
    ptr<ASTNode> root{nullptr};
    AllocatedArray<ASTNode*> nodes{};
    freopen(nullptr,"rb",stdin);
    freopen(nullptr,"wb",stdout);
    std::uint32_t node;
    std::uint32_t arg;
    while(true){
        switch(bread()){
            case 0_b: // Create root
                root = aread();
                wi<std::uint64_t>(nodes.emplace(root.get()));
                std::fflush(stdout);
                break;
            case 1_b: { // Create child
                node = ri<std::uint64_t>();
                arg = ri<std::uint64_t>();
                ptr<ASTNode> nd{aread()};
                wi<std::uint64_t>(nodes.emplace(nd.get()));
                std::fflush(stdout);
                nodes[node]->set(arg,std::move(nd));
                break;
            }
            case 2_b: // Set primitive
                node = ri<std::uint64_t>();
                arg = ri<std::uint64_t>();
                switch(bread()){
                    case 32_b: // ui32
                        nodes[node]->setprim32(arg,ri<std::uint32_t>());
                        break;
                }
                break;
            case 4_b: // Delete node
                node = ri<std::uint64_t>();
                nodes.free(node);
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
                root->compile(cc);
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

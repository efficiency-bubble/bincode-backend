#include<cppp/bfile.hpp>
#include<bbe/bbe.hpp>
#include<type_traits>
#include<cstdio>
using namespace bbe;
using namespace std::literals;
using cppp::ptr;
using asm_generic::operator ""_b;
constexpr static std::uint32_t CONTEXT_IDENTIFIER_C = 0;
constexpr static std::uint32_t CONTEXT_VALUE_C = 1;
template<typename T>
class AllocatedArray{
    std::vector<T> arr;
    bbe::impl::Allocator alloc;
    public:
        std::uint64_t emplace(T&& v){
            std::uint64_t indx = alloc.push();
            if(indx > arr.size()){
                throw std::runtime_error("allocator/array size mismatch");
            }
            if(indx == arr.size()){
                arr.emplace_back(std::move(v));
            }else{
                arr[indx] = std::move(v);
            }
            return indx;
        }
        T& operator[](std::uint64_t indx){
            return arr[indx];
        }
        const T& operator[](std::uint64_t indx) const{
            return arr[indx];
        }
        void free(std::uint64_t indx){
            alloc.pop(indx);
        }
};
char8_t read(){
    using it = std::char_traits<char8_t>::int_type;
    it ch = static_cast<it>(std::getchar());
    if(ch == std::char_traits<char8_t>::eof()){
        std::exit(0);
    }
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
    }
    throw std::runtime_error("aread(): unknown sequence");
}
template<typename T> requires(std::is_integral_v<T> && std::is_unsigned_v<T>)
T ri(){
    T ret;
    std::fread(&ret,sizeof(T),1uz,stdin);
    return ret;
}
template<typename T> requires(std::is_integral_v<T> && std::is_unsigned_v<T>)
void wi(std::type_identity_t<T> x){
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
                break;
            case 1_b: // Create child
                node = ri<std::uint64_t>();
                arg = ri<std::uint64_t>();
                nodes[node]->set(arg,aread());
                break;
            case 2_b: // Set primitive
                node = ri<std::uint64_t>();
                arg = ri<std::uint64_t>();
                switch(bread()){
                    case 32_b: // ui32
                        nodes[node]->setprim32(arg,ri<std::uint32_t>());
                        break;
                }
                break;
            case 12_b: // List autocomplete contexts
                wi<std::uint64_t>(2);
                wi<std::uint32_t>(CONTEXT_IDENTIFIER_C);
                wi<std::uint32_t>(CONTEXT_VALUE_C);
                std::fflush(stdout);
                break;
            case 40_b:{ // Compile
                    Text text;
                    FunctionCompilationContext cc{text};
                    root->compile(cc);
                    wi<std::uint64_t>(text.text().size());
                    fwrite(text.text().data(),text.text().size(),1uz,stdout);
                }
                break;
        }
    }
    return 0;
}

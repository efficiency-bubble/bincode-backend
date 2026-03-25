#include<cppp/template-string.hpp>
#include<bbe/targets/yasbepl.hpp>
#include<stdexcept>
#include<algorithm>
#include<format>
#include<list>
namespace bbe::targets::yasbepl::impl{
    namespace{
        using namespace std::literals;
        using namespace cppp::literals;
        constexpr cppp::template_string ROT{u8">0$:>1>0-+$>1+$;<"_ts};
        constexpr cppp::template_string ROR{u8'$'_ts+ROT};
        constexpr cppp::template_string ROL{ROT+u8'$'_ts};
        constexpr cppp::template_string DUP2{ROR+u8'='_ts+ROL};
        constexpr cppp::template_string SWAP{DUP2+u8"-="_ts+ROR+u8"+="_ts+ROL+u8">0-+"_ts};
        class Stack{
            std::list<const dfg::DataNode*> stack;
            cppp::str& code;
            public:
                Stack(cppp::str& code) : code(code){}
                
                const dfg::DataNode* top() const{
                    return stack.back();
                }
                const dfg::DataNode* second() const{
                    return *std::next(stack.rbegin());
                }
                void ror(){
                    stack.push_front(top());
                    stack.pop_back();
                    code.append(ROR);
                }
                void rol(){
                    stack.push_back(top());
                    stack.pop_front();
                    code.append(ROL);
                }
                void swap(){
                    std::iter_swap(stack.rbegin(),std::next(stack.rbegin()));
                    code.append(SWAP);
                }
                void bring_me(const dfg::DataNode* v){
                    while(top() != v){
                        ror();
                    }
                }
                void bring_me_in_second_place(const dfg::DataNode* v){
                    while(second() != v){
                        swap();
                        ror();
                    }
                }
                // TODO: After CSE is implemented, pop values after last use, not first
                void compile(const dfg::DataNode* node){
                    switch(node->operation()){
                        using enum dfg::NodeType;
                        case UINT32:
                            stack.push_back(node);
                            std::format_to(std::back_inserter(code),">{}"sv,node->primitive());
                            break;
                        case CALL_BUILTIN: {
                            const auto& argv = node->parents();
                            switch(node->primitive()){
                                case 10: // add32
                                    compile(argv[0uz]);
                                    compile(argv[1uz]);
                                    code.push_back(u8'+');
                                    stack.pop_back();
                                    stack.pop_back();
                                    stack.push_back(node);
                                    break;
                                case 11: // sub32
                                    compile(argv[1uz]); // reverse order
                                    compile(argv[0uz]);
                                    code.push_back(u8'-');
                                    stack.pop_back();
                                    stack.pop_back();
                                    stack.push_back(node);
                                    break;
                                default: throw std::logic_error("bbe::targets::yasbepl::Stack::compile(): Unknown magic "s+std::to_string(node->primitive()));
                            }
                            break;
                        }
                        default: throw std::logic_error("bbe::targets::yasbepl::Stack::compile(): Unknown node type "s+std::to_string(std::to_underlying(node->operation())));
                    }
                }
        };
    }
    void compile(const dfg::DataFlowGraph& dfg,cppp::str& code){
        Stack(code).compile(dfg.root());
    }
}

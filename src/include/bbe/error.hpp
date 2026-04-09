#pragma once
#include"commons.hpp"
#include<cppp/string.hpp>
#include<unordered_map>
#include<vector>
#include<span>
namespace bbe::impl{
    class ASTNode;
    class Error{
        cppp::str _reason;
        public:
            Error(cppp::str reason) : _reason(std::move(reason)){}
            const cppp::str& reason() const{
                return _reason;
            }
    };
    class ErrorDatabase{
        std::unordered_map<const ASTNode*,std::vector<Error>> records;
        public:
            void add(const ASTNode* to,cppp::str reason){
                records[to].emplace_back(std::move(reason));
            }
            void clear(){
                records.clear();
            }
            bool empty() const{
                return records.empty();
            }
            std::span<const Error> query(const ASTNode* for_node) const{
                if(auto it=records.find(for_node);it!=records.end()){
                    return it->second;
                }else{
                    return {};
                }
            }
    };
}
namespace bbe{
    BBE_EXPORT Error;
    BBE_EXPORT ErrorDatabase;
}

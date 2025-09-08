#pragma once
#include"../commons.hpp"
#include<cppp/variant.hpp>
#include<vector>
#include<cassert>
namespace bbe::inter::impl{
    struct uint32v{
        std::uint32_t value;
    };
    struct boolv{
        bool value;
    };
    struct fptr{
        std::uint32_t id;
    };
    struct pack;
    class Value{
        struct copy_construct{
            template<typename T>
            void* operator()(const T& v) const{
                return new T(v);
            }
        };
        using val_t = cppp::heap_variant<uint32v,boolv,pack,fptr>;
        val_t _value;
        public:
            Value() = default;
            template<typename T> requires(!std::is_same_v<std::remove_cvref_t<T>,Value>)
            Value(T&& v) : _value(std::forward<T>(v)){}
            Value(Value&&) = default;
            Value(const Value& other) : _value(other._value.index(),other._value?other._value.dispatch(copy_construct()):nullptr){}
            Value& operator=(Value&&) = default;
            Value& operator=(const Value& other){
                _value.reset(other._value.index(),other._value?other._value.dispatch(copy_construct()):nullptr);
                return *this;
            }
            template<typename T>
            bool holds() const{
                return _value && _value.tell() == val_t::index_of<T>;
            }
            template<typename T>
            T& get() &{
                assert(holds<T>());
                return _value.get<T>();
            }
            template<typename T>
            const T& get() const &{
                assert(holds<T>());
                return _value.get<T>();
            }
            template<typename T>
            T&& get() &&{
                assert(holds<T>());
                return std::move(_value.get<T>());
            }
            template<typename T>
            const T&& get() const &&{
                assert(holds<T>());
                return std::move(_value.get<T>());
            }
            template<typename T>
            constexpr static std::size_t index_of = val_t::index_of<T>;
            std::size_t tell() const{
                return _value.tell();
            }
            val_t& value(){
                return _value;
            }
            const val_t& value() const{
                return _value;
            }
            ~Value() = default;
    };
    struct pack{
        std::vector<Value> values;
    };
}
namespace bbe::inter{
    BBE_EXPORT uint32v;
    BBE_EXPORT boolv;
    BBE_EXPORT pack;
    BBE_EXPORT Value;
}

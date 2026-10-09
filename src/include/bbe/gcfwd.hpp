#pragma once
namespace bbe::impl{
    class EntitySweeper;
    template<typename T>
    class TraceableReference;
    template<typename T>
    bool operator==(TraceableReference<T> lhs,TraceableReference<T> rhs);
    template<typename T>
    class TraceableReference{
        mutable const T* ref;
        friend EntitySweeper;
        public:
            TraceableReference() = default;
            TraceableReference(const T& i) : ref(&i){}
            const T& operator*() const{
                return *ref;
            }
            const T* operator->() const{
                return ref;
            }
            void set(const T& other){
                ref = &other;
            }
            const T* get() const{
                return ref;
            }
            friend bool operator==(TraceableReference lhs,TraceableReference rhs){
                return lhs.ref == rhs.ref;
            }
    };
}
namespace bbe{
    BBE_EXPORT EntitySweeper;
    BBE_EXPORT TraceableReference;
}

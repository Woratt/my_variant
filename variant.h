#ifndef VARIANT_H
#define VARIANT_H

#include <iostream>
#include <new>

template<typename... Types>
class variant;

template<int N, typename T, typename... Types>
struct get_index_by_type{
    static const size_t index = -1;
};

template<int N, typename T, typename Head, typename... Tail>
struct get_index_by_type<N, T, Head, Tail...>
{
    static const size_t index = std::is_same_v<T, Head>? N : get_index_by_type<N + 1, T, Tail...>::index;
};

template<size_t N, typename T, typename... Types>
struct get_type_by_index
{
    using type = typename get_type_by_index<N - 1, Types...>::type;
};

template<typename T, typename... Types>
struct get_type_by_index<0, T, Types...>
{
    using type = T;
};

template<size_t index, typename... Types>
struct is_equal{
    static bool equal(size_t current_index, const variant<Types...>& lhs, const variant<Types...>& rhs) {
        if (current_index == index) {
            using T = typename get_type_by_index<index, Types...>::type;
            
            if constexpr (std::is_same_v<
                decltype(lhs. template get<T>()), 
                decltype(rhs. template get<T>())>) {
                return lhs.template get<T>() == rhs.template get<T>();
            } else {
                return false;
            }
        } else {
            if constexpr (index + 1 < sizeof...(Types)) {
                return is_equal<index + 1, Types...>::equal(current_index, lhs, rhs);
            } else {
                return false;
            }
        }
    }
};

template<size_t index , typename... Types>
struct copy{
    static void make_copy(size_t current, const variant<Types...>& from, variant<Types...>& to){
        if(current == index){
            using T = typename get_type_by_index<index, Types...>::type;
            to.storage. template put<index, T>(from. template get<T>()); 
            to. template set_current<index>();
        }else{
            if constexpr(index + 1 < sizeof...(Types)){
                copy<index + 1, Types...>::make_copy(current, from, to);
            }
        }
    }
};

template<size_t N, typename Visitor, typename... Types>
struct visit_s{
    static decltype(auto) make_visit(size_t current, Visitor&& visitor, const variant<Types...>& var) {
        if (current == N) {
            using T = typename get_type_by_index<N, Types...>::type;
            return visitor(var.template get<T>());
        } else {
            if constexpr (N + 1 < sizeof...(Types)) {
                return visit_s<N + 1, Visitor, Types...>::make_visit(current, std::forward<Visitor>(visitor), var);
            } else {
                throw std::runtime_error("Invalid variant index");
            }
        }
    }
};

template<size_t index , typename... Types>
struct move{
    static void make_move(size_t current, variant<Types...>& from, variant<Types...>& to){
        if(current == index){
            using T = typename get_type_by_index<index, Types...>::type;
            to.storage.template put<index, T>(std::move(from.template get<T>())); 
            to.template set_current<index>();
        }else{
            if constexpr(index + 1 < sizeof...(Types)){
                move<index + 1, Types...>::make_move(current, from, to);
            }
        }
    }
};

template<typename T, typename... Types>
struct VariantAlternative{

    using Derived = variant<Types...>;

    VariantAlternative(const T& value) noexcept{
        try{
            static_cast<Derived*>(this)->storage. template put<get_index_by_type<0, T, Types...>::index>(value);
        }catch(...){
            static_cast<Derived*>(this)->is_valueless = true;
        }
        static_cast<Derived*>(this)-> template set_current<get_index_by_type<0, T, Types...>::index>();
    }

    VariantAlternative() noexcept{}

    VariantAlternative(T&& value) noexcept{
        try{
            static_cast<Derived*>(this)->storage. template put<get_index_by_type<0, T, Types...>::index>(std::move(value));
        }catch(...){
            static_cast<Derived*>(this)->is_valueless = true;
        }
        static_cast<Derived*>(this)-> template set_current<get_index_by_type<0, T, Types...>::index>();
    }

    void destroy(){
        auto this_ptr = static_cast<Derived*>(this);
        if(get_index_by_type<0, T, Types...>::index == this_ptr->current){
            this_ptr->storage. template des<get_index_by_type<0, T, Types...>::index, T>();
        }
    }
};

template<typename... Types>
class variant : public VariantAlternative<Types, Types...>...{
private:
    template<typename... Tail>
    union VarUnion{
        template<size_t N, typename T>
    void put(const T&) {}

    template<size_t N, typename T>
    void put(T&&) noexcept {}

    template<size_t N, typename T>
    void des(){}
    };

    template<typename Head, typename... Tail>
    union VarUnion<Head, Tail...>
    {
        Head head;
        VarUnion<Tail...> tail;

        VarUnion(){};
        ~VarUnion(){};

        template<size_t N, typename T>
        void put(const T& value){
            if constexpr(N == 0){
                new (&head) T(value);
            }else{
                tail. template put<N-1>(value);
            }
        }
        template<size_t N, typename T>
        void put(T&& value) noexcept{
            if constexpr(N == 0){
                new (&head) T(std::forward<T>(value));
            }else{
                tail. template put<N-1>(std::move(value));
            }
        }
        template<size_t N ,typename T>
        T& get_value() noexcept{
            if constexpr(N == 0){
                return head;
            }else{
                return tail. template get_value<N-1, T>();
            }
        }
        template<size_t N ,typename T>
        const T& get_value() const noexcept{
            if constexpr(N == 0){
                return head;
            }else{
                return tail. template get_value<N-1, T>();
            }
        }

        template<size_t N, typename T>
        void des(){
            if constexpr(N == 0){
                head.~T();
            }else{
                tail. template des<N-1, T>();
            }
        }
    };
    
    size_t current;
    VarUnion<Types...> storage;
    bool is_valueless = false;

    template<typename T, typename... AllTypes>
    friend struct VariantAlternative;

    template<size_t index , typename... AllTypes>
    friend struct copy;

    template<size_t index , typename... AllTypes>
    friend struct move;

public:

    using VariantAlternative<Types, Types...>::VariantAlternative...;

    const size_t& index() const{
        return current;
    }

    variant(const variant& other){
        try{
            copy<0, Types...>::make_copy(other.current, other, *this);
        }catch(...){
            is_valueless = true;
            throw;
        }
    }

    variant(variant&& other) noexcept {
        try{
            move<0, Types...>::make_move(other.current, other, *this);
        }catch(...){
            is_valueless = true;
        }
    }    

    template<size_t N>
    void set_current() noexcept{
        current = N;
    }

    bool operator==(const variant& other) const noexcept{
        if (current != other.current){
            return false;
        }
        else{
            return is_equal<0, Types...>::equal(other.current, other, *this);
        }
    }

    bool operator!=(const variant& other) const noexcept{
        return !(*this == other);
    }

    bool valueless_by_exception() const {
        return is_valueless;
    }

    template<typename T>
    variant& operator=(const T& value){
        (VariantAlternative<Types, Types...>::destroy(), ...);
        try{
            this->storage. template put<get_index_by_type<0, T, Types...>::index>(value);
        }catch(...){
            is_valueless = true;
            throw;
        }
        current = get_index_by_type<0, T, Types...>::index;
        return *this;
    }

    variant& operator=(const variant& other){
        if(this != &other){
            (VariantAlternative<Types, Types...>::destroy(), ...);
            try{
                copy<0, Types...>::make_copy(other.current, other, *this);
            }catch(...){
                is_valueless = true;
                throw;
            }
        }
        return *this;
    }

    template<typename T>
    T& get() {
        static_assert((get_index_by_type<0, T, Types...>::index != -1), "not faund type");
        if (current == get_index_by_type<0, T, Types...>::index){
            return storage. template get_value<get_index_by_type<0, T, Types...>::index, T>();
        }else{
            throw std::bad_variant_access{};
        }
    } 

    template<typename T>
    const T& get() const {
        static_assert((get_index_by_type<0, T, Types...>::index != -1), "not faund type");
        if (current == get_index_by_type<0, T, Types...>::index){
            return storage. template get_value<get_index_by_type<0, T, Types...>::index, T>();
        }else{
            throw std::bad_variant_access{};
        }
    }

    template<typename T>
    bool holds_alternative() const {
        return get_index_by_type<0, T, Types...>::index == current;
    }

    template<typename Visitor>
    constexpr decltype(auto) visit(Visitor&& visitor){
        return visit_s<0, Visitor, Types...>::make_visit(current, std::forward<Visitor>(visitor), *this);
    }

    ~variant(){
        (VariantAlternative<Types, Types...>::destroy(), ...);
    }

};

#endif
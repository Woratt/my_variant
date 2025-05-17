#include <iostream>
#include <variant>
#include <new>

template<typename... Types>
class variant;

template<int N, typename T, typename... Types>
struct get_index_by_type{
    static const int index = -1;
};

template<int N, typename T, typename Head, typename... Tail>
struct get_index_by_type<N, T, Head, Tail...>
{
    static const int index = std::is_same_v<T, Head>? N : get_index_by_type<N + 1, T, Tail...>::index;
};

template<int N,typename T, typename... Types>
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

template<typename T, typename... Types>
struct VariantAlternative{

    using Derived = variant<Types...>;

    VariantAlternative(const T& value) noexcept{
        static_cast<Derived*>(this)->storage. template put<get_index_by_type<0, T, Types...>::index>(value);
        static_cast<Derived*>(this)->current = get_index_by_type<0, T, Types...>::index;
    }

    VariantAlternative() noexcept{}

    VariantAlternative(T&& value) noexcept{
        static_cast<Derived*>(this)->storage. template put<get_index_by_type<0, T, Types...>::index>(std::move(value));
        static_cast<Derived*>(this)->current = get_index_by_type<0, T, Types...>::index;
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
                tail. template get_value<N-1, T>();
            }
        }
        template<size_t N ,typename T>
        const T& get_value() const noexcept{
            if constexpr(N == 0){
                return head;
            }else{
                tail. template get_value<N-1, T>();
            }
        }

        template<size_t N, typename T>
        void des(){
            if constexpr(N == 0){
                std::cout << "destroy\n";
                head.~T();
            }else{
                tail. template des<N-1, T>();
            }
        }
    };
    
    public:
    static size_t current;
    VarUnion<Types...> storage;
    size_t index() const{
        return current;
    }

    //template<typename T>
    bool operator==(const variant& other) const noexcept{
        if (current != other.current){
            return false;
        }
        else{
            return is_equal<0, Types...>::equal(other.current, other, *this);
        }
    }

    template<typename T>
    variant& operator=(const T& value) noexcept{
        (VariantAlternative<Types, Types...>::destroy(), ...);
        this->storage. template put<get_index_by_type<0, T, Types...>::index>(value);
        current = get_index_by_type<0, T, Types...>::index;
        return *this;
    }


    /*variant& operator=(const variant& other) noexcept{
        if()
        (VariantAlternative<Types, Types...>::destroy(), ...);
        this->storage. template put<get_index_by_type<0, decltype(other->storage. template get_value<>), Types...>::index>(other->get<T>());
        current = get_index_by_type<0, T, Types...>::index;
    }*/

    template<typename T>
    T& get() {
        if (current == get_index_by_type<0, T, Types...>::index){
            return storage. template get_value<get_index_by_type<0, T, Types...>::index, T>();
        }else{
            throw std::bad_alloc();
        }
    } 
    template<typename T>
    const T& get() const {
        if (current == get_index_by_type<0, T, Types...>::index){
            return storage. template get_value<get_index_by_type<0, T, Types...>::index, T>();
        }else{
            throw std::bad_alloc();
        }
    } 

    template<typename T>
    bool holds_alternative() const {
        return get_index_by_type<0, T, Types...>::index == current;
    }

    using VariantAlternative<Types, Types...>::VariantAlternative...;

    ~variant(){
        (VariantAlternative<Types, Types...>::destroy(), ...);
    }

};
template<typename... Types>
size_t variant<Types...>::current = 0;

union B{
    std::string s;
    int i;

    B(){};
    ~B(){};
};

class S{
    public:
    size_t* t;

    S(const S&) = delete;
    S& operator=(const S&) = delete;
    S(){};

    S(S&& other) noexcept : t(other.t) {
        other.t = nullptr;
    }

    S& operator=(S&& other) noexcept{
        if(!this){
            delete t;
        }
        t = other.t;
        other.t = nullptr;
        return *this;
    }

    S(const size_t& t): t(new size_t(t)){}
    S(size_t&& t): t(new size_t(t)){};
    ~S(){
       delete t;
    }
};


int main(){
    variant<int, double> f(3);
    variant<int, double> g(7);
    f = g;
    std::cout << (f == g) << "\n";
    //std::variant<int, double> f;
    //std::variant<int, double> g;
    //std::cout << (f == g) << "\n";

    return 0;
}
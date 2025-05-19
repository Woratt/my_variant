#include <iostream>
#include <variant>
#include <new>

template<typename... Types>
class variant;

template<int N, typename T, typename... Types>
struct get_index_by_type{
    static const size_t index = 0;
};

template<int N, typename T, typename Head, typename... Tail>
struct get_index_by_type<N, T, Head, Tail...>
{
    static const size_t index = std::is_same_v<T, Head>? N : get_index_by_type<N + 1, T, Tail...>::index;
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

template<size_t index , typename... Types>
struct move{
    static void make_move(size_t current, variant<Types...>&& from, variant<Types...>& to){
        if(current == index){
            using T = typename get_type_by_index<index, Types...>::type;
            to.storage.template put<index, T>(std::move(from.template get<T>())); 
            to.template set_current<index>();
        }else{
            if constexpr(index + 1 < sizeof...(Types)){
                move<index + 1, Types...>::make_move(current, std::move(from), to);
            }
        }
    }
};

template<typename T, typename... Types>
struct VariantAlternative{

    using Derived = variant<Types...>;

    VariantAlternative(const T& value) noexcept{
        static_cast<Derived*>(this)->storage. template put<get_index_by_type<0, T, Types...>::index>(value);
        static_cast<Derived*>(this)-> template set_current<get_index_by_type<0, T, Types...>::index>();
    }

    VariantAlternative() noexcept{}

    VariantAlternative(T&& value) noexcept{
        static_cast<Derived*>(this)->storage. template put<get_index_by_type<0, T, Types...>::index>(std::move(value));
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

    template<typename T, typename... AllTypes>
    friend struct VariantAlternative;

    template<size_t index , typename... AllTypes>
    friend struct copy;

    template<size_t index , typename... AllTypes>
    friend struct move;

    public:
    size_t index() const{
        return current;
    }

    variant(const variant& other) noexcept {
        copy<0, Types...>::make_copy(other.current, other, *this);
    }

    variant(variant&& other) noexcept {
        move<0, Types...>::make_move(other.current, std::move(other), *this);
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

    template<typename T>
    variant& operator=(const T& value) noexcept{
        (VariantAlternative<Types, Types...>::destroy(), ...);
        this->storage. template put<get_index_by_type<0, T, Types...>::index>(value);
        current = get_index_by_type<0, T, Types...>::index;
        return *this;
    }


    variant& operator=(const variant& other) noexcept{
        if(this != &other){
            (VariantAlternative<Types, Types...>::destroy(), ...);
            copy<0, Types...>::make_copy(other.current, other, *this);
        }
        return *this;
    }

    template<typename T>
    T& get() {       
        if (current == get_index_by_type<0, T, Types...>::index){
            return storage. template get_value<get_index_by_type<0, T, Types...>::index, T>();
        }else{
            throw std::bad_variant_access{};
        }
    } 
    template<typename T>
    const T& get() const {
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

    using VariantAlternative<Types, Types...>::VariantAlternative...;

    ~variant(){
        (VariantAlternative<Types, Types...>::destroy(), ...);
    }

};

using std::string;

void test_basic_assignment_and_get() {
    variant<int, double, string> v;
    v = 42;
    assert(v.holds_alternative<int>());
    assert(v.get<int>() == 42);

    v = 3.14;
    assert(v.holds_alternative<double>());
    assert(v.get<double>() == 3.14);

    v = string("hello");
    assert(v.holds_alternative<string>());
    assert(v.get<string>() == "hello");
}

void test_copy_constructor() {
    variant<int, double, string> v1;
    v1 = string("copy");

    variant<int, double, string> v2(v1);

    assert(v2.holds_alternative<string>());
    assert(v2.get<string>() == "copy");
}

void test_assignment_operator() {
    variant<int, double, string> v1;
    v1 = 5;

    variant<int, double, string> v2;
    v2 = string("text");

    v2 = v1;

    assert(v2.holds_alternative<int>());
    assert(v2.get<int>() == 5);
}

void test_equality_operator() {
    variant<int, double, string> v1 = 10;
    variant<int, double, string> v2 = 10;
    variant<int, double, string> v3 = string("10");

    assert(v1 == v2);
    assert(v1 != v3);
}

void test_move_constructor() {
    variant<int, double, string> v1;
    v1 = string("moved");

    variant<int, double, string> v2(std::move(v1));

    assert(v2.holds_alternative<string>());
    assert(v2.get<string>() == "moved");
}

void test_move_assignment() {
    variant<int, double, string> v1 = 123;
    variant<int, double, string> v2;
    v2 = std::move(v1);

    assert(v2.holds_alternative<int>());
    assert(v2.get<int>() == 123);
}

void test_bad_variant_access() {
    variant<int, double, string> v;
    v = 3.14;

    try {
        v.get<int>();
        assert(false); 
    } catch (const std::bad_variant_access&) {
        assert(true); 
    }
}

int main() {
    test_basic_assignment_and_get();
    test_copy_constructor();
    test_assignment_operator();
    test_equality_operator();
    test_move_constructor();
    test_move_assignment();
    test_bad_variant_access();

    std::cout << "Усі тести пройдені успішно.\n";
    return 0;
}



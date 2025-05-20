#include <iostream>
#include <new>
#include "variant.h"

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

    variant<int, double> v(3);
    variant<int, double> f(v);
    std::cout << v.valueless_by_exception() << "\n";
    std::cout << v.get<int>() << "\n";
    std::cout << f.valueless_by_exception() << "\n";
    std::cout << f.get<int>() << "\n";
    return 0;
}
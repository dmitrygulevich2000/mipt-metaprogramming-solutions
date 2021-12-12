#ifndef MIPT_METAPROGRAMMING_USEFUL_HPP
#define MIPT_METAPROGRAMMING_USEFUL_HPP

#include "reflect.hpp"


namespace TypeTupleTest {
    // For debug
    template<class T>
    constexpr bool DependentFalse = false;

    using namespace TypeTuples;

    // Helpers
    template<class T>
    using Starred = T*;

    template<class A, class B>
    struct SizeEq {
        static constexpr bool Value = (sizeof(A) == sizeof(B));
    };

    template<class A, class B>
    struct Bigger {
        using Type = std::conditional_t<(sizeof(A) > sizeof(B)), A, B>;
    };

    template<class A, class B>
    struct Second {
        using Type = B;
    };

    template<class A, class B>
    struct First {
        using Type = A;
    };

    template<class A>
    struct Small {
        static constexpr bool Value = (sizeof(A) <= 1);
    };

    // Tests
    static_assert(std::is_same_v<
            Get<0, TTuple<int, char>>,
            int>);
    static_assert(std::is_same_v<
            Get<1, TTuple<int, char>>,
            char>);
    static_assert(std::is_same_v<
            Append<char, TTuple<int>>,
            TTuple<int, char>>);
    static_assert(std::is_same_v<
            Map<Starred, TTuple<int, char>>,
            TTuple<int*, char*>>);
    static_assert(std::is_same_v<
            Fold<Bigger, bool, TTuple<int, char>>,
            int>);
    static_assert(std::is_same_v<
            GroupBy<SizeEq, TTuple<int, unsigned int, char, bool, long long>>,
            TTuple<TTuple<int, unsigned int>, TTuple<char, bool>, TTuple<long long>>>);
    static_assert(std::is_same_v<
            Map<BindFold<Second, int*>::template F, GroupBy<SizeEq, TTuple<int, unsigned int, char, bool, long long>>>,
            TTuple<unsigned int, bool, long long>>);
    static_assert(std::is_same_v<
            Map<BindFold<First, int*>::template F, GroupBy<SizeEq, TTuple<int, unsigned int, char, bool, long long>>>,
            TTuple<int*, int*, int*>>);
    static_assert(std::is_same_v<
            Filter<Small, TTuple<signed char, int, bool, long>>,
            TTuple<signed char, bool>>);

} // namespace TypeTupleTest

template<template <typename> typename T, class Arg>
auto IsTemplateTypeHelper(T<Arg>) {
    return Arg{};
}

template<class T>
struct IsTemplateType {
    static constexpr bool Value = requires(T t) { IsTemplateTypeHelper(t); };
};

template<class T>
requires IsTemplateType<T>::Value
struct TemplateArgument {
    using Type = decltype(TemplateTypeHelper(T{}));
};

#endif //MIPT_METAPROGRAMMING_USEFUL_HPP

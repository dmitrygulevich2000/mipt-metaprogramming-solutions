#ifndef MIPT_METAPROGRAMMING_TYPE_LISTS_HPP
#define MIPT_METAPROGRAMMING_TYPE_LISTS_HPP

#include <concepts>
#include <type_traits>

namespace TypeTuples {
    template<class... Ts>
    struct TTuple {};

    template<class ... Ts>
    void TypeTupleTest(TTuple<Ts...>) {}

    template<class TT>
    concept TypeTuple = requires(TT t) { TypeTupleTest(t); };
}  // namespace TypeTuples

namespace TypeLists {
    constexpr bool NotInstantiated = false;

    template<class TL>
    concept TypeSequence =
    requires {
        typename TL::Head;
        typename TL::Tail;
    };

    struct Nil {};

    template<class TL>
    concept Empty = std::derived_from<TL, Nil>;

    template<class TL>
    concept TypeList = Empty<TL> || TypeSequence<TL>;


    // Cons
    template<class T, TypeList TL>
    struct Cons {
        using Head = T;
        using Tail = TL;
    };
    // end Cons

    // Iterate
    template< template <typename> typename F, class T>
    struct Iterate {
        using Head = T;
        using Tail = Iterate<F, F<T>>;
    };
    // end Iterate

    // Repeat
    template<class T>
    using Id = T;

    template<class T>
    using Repeat = Iterate<Id, T>;
    // end Repeat

    // FromTuple
    template<class ... Ts>
    struct FromTuple {};

    template<class ... Ts>
    // empty TTuple
    struct FromTuple<TypeTuples::TTuple<Ts...>>: Nil {};

    template<class T, class ... Ts>
    struct FromTuple<TypeTuples::TTuple<T, Ts...>>:
            Cons<T, FromTuple<TypeTuples::TTuple<Ts...>>> {
    };
    // end FromTuple

    // ToTuple
    template<class A, class B>
    struct ToTupleImpl;

    template<TypeList TL, typename ... Ts>
    struct ToTupleImpl<TL, TypeTuples::TTuple<Ts...>> {
        using Type = typename ToTupleImpl<typename TL::Tail, TypeTuples::TTuple<Ts..., typename TL::Head>>::Type;
    };

    template<Empty TL, typename ... Ts>
    struct ToTupleImpl<TL, TypeTuples::TTuple<Ts...>> {
        using Type = TypeTuples::TTuple<Ts...>;
    };

    template<TypeList TL>
    using ToTuple = typename ToTupleImpl<TL, TypeTuples::TTuple<>>::Type;
    // end ToTuple

    // Take
    template<int N, TypeList TL>
    struct Take: Cons<typename TL::Head, Take<N-1, typename TL::Tail>> {};

    template<TypeList TL>
    struct Take<0, TL>: Nil {};

    template<int N, Empty TL>
    struct Take<N, TL>: Nil {};

    template<Empty TL>
    struct Take<0, TL>: Nil {};
    // end Take

    // Drop
    template<int N, TypeList TL>
    struct DropImpl {
        using Type = typename DropImpl<N-1, typename TL::Tail>::Type;
    };

    template<TypeList TL>
    struct DropImpl<0, TL> {
        using Type = TL;
    };

    template<int N, Empty TL>
    struct DropImpl<N, TL> {
        using Type = Nil;
    };

    template<Empty TL>
    struct DropImpl<0, TL> {
        using Type = Nil;
    };

    template<int N, TypeList TL>
    using Drop = typename DropImpl<N, TL>::Type;
    // end Drop

    // Replicate
    template<int N, class T>
    using Replicate = Take<N, Repeat<T>>;
    // end Replicate

    // Cycle
    template<class A>
    struct CycleImpl;

    template<class T, class ... Ts>
    struct CycleImpl<TypeTuples::TTuple<T, Ts...>> {
        using Head = T;
        using Tail = CycleImpl<TypeTuples::TTuple<Ts..., T>>;
    };

    template<TypeList TL>
    using Cycle = CycleImpl<ToTuple<TL>>;
    // end Cycle

    // Map
    template<template<typename> typename F, TypeList TL>
    struct Map {
        using Head = F<typename TL::Head>;
        using Tail = Map<F, typename TL::Tail>;
    };

    template<template<typename> typename F, Empty TL>
    struct Map<F, TL>: Nil {
    };
    // end Map

    // Filter
    template<template <typename> typename P, TypeList TL>
    struct DropNegatives;

    template<bool B, template <typename> typename P, TypeList TL>
    // B == false
    struct CondEvaluated {
        using Type = typename DropNegatives<P, typename TL::Tail>::Type;
    };

    template<template <typename> typename P, TypeList TL>
    struct CondEvaluated<true, P, TL> {
        using Type = TL;
    };

    template<template <typename> typename P, TypeList TL>
    struct DropNegatives {
        using Type = typename CondEvaluated<P<typename TL::Head>::Value, P, TL>::Type;
    };

    template<template <typename> typename P, Empty TL>
    struct DropNegatives<P, TL> {
        using Type = Nil;
    };

    template<template <typename> typename P, TypeList Dropped>
    struct FilterImpl {
        using Head = typename Dropped::Head;
        using Tail = FilterImpl<P, typename DropNegatives<P, typename Dropped::Tail>::Type>;
    };

    template<template <typename> typename P, Empty Dropped>
    struct FilterImpl<P, Dropped>: Nil {};

    template<template <typename> typename P, TypeList TL>
    using Filter = FilterImpl<P, typename DropNegatives<P, TL>::Type>;
    // end Filter

    // GroupBy
    template<bool HeadInGroup, template <typename, typename> typename EQ, TypeList TL, class ... Grouped>
    struct MoveHead;

    template<template <typename, typename> typename EQ, TypeList TL, class ... Grouped>
    // empty group
    struct GetGroup {
        using Type = typename GetGroup<EQ, typename TL::Tail, typename TL::Head, Grouped...>::Type;
    };

    template<template <typename, typename> typename EQ, TypeList TL, class GrLeader, class ... Grouped>
    struct GetGroup<EQ, TL, GrLeader, Grouped...> {
        using Type = typename MoveHead<EQ<GrLeader, typename TL::Head>::Value, EQ, TL, Grouped..., GrLeader>::Type;
    };

    template<template <typename, typename> typename EQ, Empty TL, class GrLeader, class ... Grouped>
    struct GetGroup<EQ, TL, GrLeader, Grouped...> {
        using Type = Cons<FromTuple<TypeTuples::TTuple<Grouped..., GrLeader>>, Nil>;
    };

    template<template <typename, typename> typename EQ, Empty TL, class ... Grouped>
    // empty group
    struct GetGroup<EQ, TL, Grouped...> {
        using Type = Nil;
    };

    template<bool HeadInGroup, template <typename, typename> typename EQ, TypeList TL, class ... Grouped>
    // HeadInGroup == true
    struct MoveHead {
        using Type = typename GetGroup<EQ, typename TL::Tail, typename TL::Head, Grouped...>::Type;
    };

    template<template <typename, typename> typename EQ, TypeList TL, class ... Grouped>
    struct MoveHead<false, EQ, TL, Grouped...> {
        using Type = Cons<FromTuple<TypeTuples::TTuple<Grouped...>>, TL>;
    };

    template<template <typename, typename> typename EQ, TypeList Grouped>
    struct GroupByImpl {
        using Head = typename Grouped::Head;
        using Tail = GroupByImpl<EQ, typename GetGroup<EQ, typename Grouped::Tail>::Type>;
    };

    template<template <typename, typename> typename EQ, Empty TL>
    struct GroupByImpl<EQ, TL>: Nil {};

    template<template <typename, typename> typename EQ, TypeList TL>
    using GroupBy = GroupByImpl<EQ, typename GetGroup<EQ, TL>::Type>;
    // end GroupBy

    // Inits
    template<TypeList TL, class ... Prefix>
    struct Inits {
        using Head = FromTuple<TypeTuples::TTuple<Prefix...>>;
        using Tail = Inits<typename TL::Tail, Prefix ..., typename TL::Head>;
    };

    template<Empty TL, class ... Prefix>
    struct Inits<TL, Prefix...> {
        using Head = FromTuple<TypeTuples::TTuple<Prefix...>>;
        using Tail = Nil;
    };
    // end Inits

    // Tails
    template<TypeList TL>
    struct Tails {
        using Head = TL;
        using Tail = Tails<typename TL::Tail>;
    };

    template<Empty TL>
    struct Tails<TL> {
        using Head = Nil;
        using Tail = Nil;
    };
    // end Tails

    // Scanl
    template<template<typename, typename> typename OP,  class Prev, TypeList TL>
    struct ScanlImpl {
        using Head = OP<Prev, typename TL::Head>;
        using Tail = ScanlImpl<OP, Head, typename TL::Tail>;
    };

    template<template<typename, typename> typename OP,  class Prev, Empty TL>
    struct ScanlImpl<OP, Prev, TL>: Nil {};

    template<template<typename, typename> typename OP,  class T, TypeList TL>
    using Scanl = Cons<T, ScanlImpl<OP, T, TL>>;
    // end Scanl

    // Foldl
    template<template<typename, typename > typename OP, class Prev, TypeList TL>
    struct FoldlImpl {
        using Type = typename FoldlImpl<OP, OP<Prev, typename TL::Head>, typename TL::Tail> ::Type;
    };

    template<template<typename, typename > typename OP, class Prev, Empty TL>
    struct FoldlImpl<OP, Prev, TL> {
        using Type = Prev;
    };

    template<template<typename, typename > typename OP, class T, TypeList TL>
    using Foldl = typename FoldlImpl<OP, T, TL>::Type;
    // end Foldl

    // Zip2
    template<TypeList L, TypeList R>
    struct Zip2 {
        using Head = TypeTuples::TTuple<typename L::Head, typename R::Head>;
        using Tail = Zip2<typename L::Tail, typename R::Tail>;
    };

    template<TypeList L, Empty R>
    struct Zip2<L, R>: Nil {};

    template<Empty L, TypeList R>
    struct Zip2<L, R>: Nil {};

    template<Empty L, Empty R>
    struct Zip2<L, R>: Nil {};
    // end Zip2

    // Zip
    template<typename ... TL>
    struct Zip: Nil {};

    template<TypeSequence ... TL>
    struct Zip<TL...> {
        using Head = TypeTuples::TTuple<typename TL::Head ...>;
        using Tail = Zip<typename TL::Tail...>;
    };
    // end Zip

}  // namespace TypeLists

// Integer Sequences
using TypeLists::Iterate;

template<auto V>
struct ValueTag{ static constexpr auto Value = V; };

template<class VT>
concept VTag = requires { VT::Value; };

template<class T, T... ts>
using VTuple = TypeTuples::TTuple<ValueTag<ts>...>;

// Nats
template<VTag VT>
    requires std::is_integral_v<decltype(VT::Value)>
using Incr = ValueTag<VT::Value + 1>;

using Nats = Iterate<Incr, ValueTag<0>>;
// end Nats

// Fib
using TypeLists::Cons;

template< template <typename, typename> typename Rel, class Prev, class Curr>
struct RecurrentOrder2Impl {
    using Head = Rel<Prev, Curr>;
    using Tail = RecurrentOrder2Impl<Rel, Curr, Head>;
};

template< template <typename, typename> typename Rel, class T0, class T1>
using RecurrentOrder2 = Cons<T0, Cons<T1, RecurrentOrder2Impl<Rel, T0, T1>>>;

template<VTag V1, VTag V2>
    requires std::is_integral_v<decltype(V1::Value)> && std::is_integral_v<decltype(V2::Value)>
using Sum = ValueTag<V1::Value + V2::Value>;

using Fib = RecurrentOrder2<Sum, ValueTag<0>, ValueTag<1>>;
// end Fib

// Prime
using TypeLists::Map;
using TypeLists::GroupBy;
using TypeLists::TypeList;
using TypeLists::Take;
using TypeLists::Drop;

// primality test
template<int x, int divider>
struct IsPrimeImpl {
    constexpr static bool Value = !(x % divider == 0) && IsPrimeImpl<x, divider + 1>::Value;
};

template<int x, int divider>
    requires (divider*divider > x)
struct IsPrimeImpl<x, divider> {
    constexpr static bool Value = true;
};

template<int divider>
struct IsPrimeImpl<2, divider> {
    constexpr static bool Value = true;
};

template<>
struct IsPrimeImpl<2, 2> {
    constexpr static bool Value = true;
};

template<int x>
requires (x >= 2)
constexpr bool IsPrime = IsPrimeImpl<x, 2>::Value;
// end primality test

template<VTag V1, VTag V2>
    requires std::is_integral_v<decltype(V2::Value)>
struct SecondIsNotPrime { constexpr static bool Value = !IsPrime<V2::Value>; };

template<TypeLists::TypeList TL>
using First = typename TL::Head;

using Primes = Map<First, GroupBy<SecondIsNotPrime, Drop<2, Nats>>>;
// end Prime

#endif //MIPT_METAPROGRAMMING_TYPE_LISTS_HPP
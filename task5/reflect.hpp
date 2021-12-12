#ifndef MIPT_METAPROGRAMMING_REFLECT_HPP
#define MIPT_METAPROGRAMMING_REFLECT_HPP

#include <cstddef>
#include <type_traits>
#include <utility>


//----------------------- TYPE TUPLES
namespace TypeTuples {
    // TTuple type and concept
    template<class... Ts>
    struct TTuple {};

    template<class ... Ts>
    void TypeTupleTest(TTuple<Ts...>) {}

    void EmptyTest(TTuple<>) {}

    template<class TT>
    concept TypeTuple = requires(TT t) { TypeTupleTest(t); };

    template<class TT>
    concept EmptyTT = TypeTuple<TT> && requires(TT t) { EmptyTest(t); };

    //-------------- operations
    // Size
    template<TypeTuple TT>
    struct SizeImpl {};

    template<class ... Ts>
    struct SizeImpl<TTuple<Ts...>> {
        static constexpr size_t Value = sizeof...(Ts);
    };

    template<TypeTuple TT>
    constexpr size_t Size = SizeImpl<TT>::Value;

    // Append
    template<class T, class ... Ts>
    auto AppendHelper(TTuple<Ts...>) {
        return TTuple<Ts..., T>{};
    }

    template<class T, TypeTuple TT>
    using Append = decltype(AppendHelper<T>(TT{}));

    // Get
    template<size_t i, TypeTuple TT> // empty TT
    struct GetImpl {};

    template<size_t i, class T, class ... Ts>
    struct GetImpl<i, TTuple<T, Ts...>> {
        using This = typename GetImpl<i - 1, TTuple<Ts...>>::This;
    };

    template<class T, class ... Ts>
    struct GetImpl<0, TTuple<T, Ts...>> {
        using This = T;
    };

    template<size_t i, TypeTuple TT>
    using Get = typename GetImpl<i, TT>::This;

    // Head
    template<TypeTuple TT>
    struct HeadImpl {};

    template<class T, class ... Ts>
    struct HeadImpl<TTuple<T, Ts...>> {
        using This = T;
    };

    template<TypeTuple TT>
    using Head = typename HeadImpl<TT>::This;

    // Map
    template<template<typename> typename F, class ... Ts>
    constexpr auto MapHelper(TTuple<Ts...>) {
        return TTuple<F<Ts>...>{};
    }

    template<template<typename> typename F, TypeTuple TT>
    using Map = decltype(MapHelper<F>(TT{}));

    // Fold
    template<template<typename, typename> typename OP, class T, TypeTuple TT>  // empty TT
    struct FoldImpl {
        using This = T;
    };

    template<template<typename, typename> typename OP, class T, class HeadT, class ...Ts>
    struct FoldImpl<OP, T, TTuple<HeadT, Ts...>> {
        using This = typename FoldImpl<OP, typename OP<T, HeadT>::Type, TTuple<Ts...>>::This;
    };

    template<template<typename, typename> typename OP, class T, TypeTuple TT>
    using Fold = typename FoldImpl<OP, T, TT>::This;

    template<template <typename, typename> typename OP, class T>
    struct BindFold {
        template<TypeTuple TT>
        using F = Fold<OP, T, TT>;
    };

    // GroupBy
    template<template <typename, typename> typename EQ, TypeTuple Result,
        TypeTuple Tail, class ... InGroup> // empty Tail, empty Group
    struct GroupByImpl {
        using This = Result;
    };

    template<template <typename, typename> typename EQ, TypeTuple Result,
            class ... InTail, class G, class ... InGroup> // empty Tail
    struct GroupByImpl<EQ, Result, TTuple<InTail...>, G, InGroup...> {
        using This = Append<TTuple<InGroup..., G>, Result>;
    };

    template<template <typename, typename> typename EQ, TypeTuple Result,
            class T, class ... InTail, class ... InGroup> // empty Group
    struct GroupByImpl<EQ, Result, TTuple<T, InTail...>, InGroup...> {
        using This = typename GroupByImpl<EQ, Result, TTuple<InTail...>, T, InGroup...>::This;
    };

    template<template <typename, typename> typename EQ, TypeTuple Result,
            class T, class ... InTail, class G, class ... InGroup>
    requires EQ<G, T>::Value
    struct GroupByImpl<EQ, Result, TTuple<T, InTail...>, G, InGroup...> {
        using This = typename GroupByImpl<EQ, Result, TTuple<InTail...>, T, InGroup..., G>::This;
    };

    template<template <typename, typename> typename EQ, TypeTuple Result,
            class T, class ... InTail, class G, class ... InGroup>
    requires (!EQ<G, T>::Value)
    struct GroupByImpl<EQ, Result, TTuple<T, InTail...>, G, InGroup...> {
        using This = typename GroupByImpl<EQ, Append<TTuple<InGroup..., G>, Result>, TTuple<InTail...>, T>::This;
    };

    template<template <typename, typename> typename EQ, TypeTuple TT>
    using GroupBy = typename GroupByImpl<EQ, TTuple<>, TT>::This;

    // Filter
    template<template <typename> typename P, TypeTuple TT, class ... Checked> // empty TT
    struct FilterImpl {
        using This = TTuple<Checked...>;
    };

    template<template <typename> typename P, class T, class ... Ts, class ... Checked>
            requires P<T>::Value
    struct FilterImpl<P, TTuple<T, Ts...>, Checked...> {
        using This = typename FilterImpl<P, TTuple<Ts...>, Checked..., T>::This;
    };

    template<template <typename> typename P, class T, class ... Ts, class ... Checked>
    requires (!P<T>::Value)
    struct FilterImpl<P, TTuple<T, Ts...>, Checked...> {
        using This = typename FilterImpl<P, TTuple<Ts...>, Checked...>::This;
    };

    template<template <typename> typename P, TypeTuple TT>
    using Filter = typename FilterImpl<P, TT>::This;

}  // namespace TypeTuples

using namespace TypeTuples;


//----------------------- REFLECTION
// Field counting
template <class T, class... Args>
concept AggregateConstructibleFrom = requires(Args... args) {
    T{ args... };
};

template <std::size_t I>
struct UbiqConstructor {
    template <class Type>
    constexpr operator Type&() const noexcept;
};

template <class T, std::size_t... I>
constexpr size_t countFieldsImpl(std::index_sequence<I...>) {
    return sizeof...(I) - 1;
}

template <class T, std::size_t... I> requires
    AggregateConstructibleFrom<T, UbiqConstructor<I>...>
constexpr size_t countFieldsImpl(std::index_sequence<I...>) {
    return countFieldsImpl<T>(std::index_sequence<0, I...>{});
}

template <class T>
constexpr size_t countFields() {
    return countFieldsImpl<T>(std::index_sequence<>{});
}
// end Field counting

// Field types
template <class T, int N>
struct Tag {
    friend auto loophole(Tag<T, N>);
};

template<class T, int N, class F>
struct LoopholeSet {
    friend auto loophole(Tag<T, N>) { return F{}; };
};

template <class T, std::size_t I>
struct LoopholeUbiq {
    template <class Type>
    constexpr operator Type() const noexcept {
        LoopholeSet<T, I, Type> unused{};
        return {};
    };
};

template<class T, int N>
struct LoopholeGet {
    using Type = decltype(loophole(Tag<T, N>{}));
};

template <class T, size_t... Is>
constexpr auto asTTupleImpl(std::index_sequence<Is...>) {
    constexpr T t{ LoopholeUbiq<T, Is>{}... };
    return TTuple<typename LoopholeGet<T, Is>::Type...>{};
}

template <class T>
constexpr auto asTTuple() {
    constexpr size_t num_fields = countFields<T>();
    return asTTupleImpl<T>(std::make_index_sequence<num_fields>{});
}
// end Field types


//----------------------- MAIN SOLUTION
template <class...>
class Annotate {};

template<class T, class ... annotations>
struct FStat;

template<class ... as>
void AnnotateTest(Annotate<as...>) {};

template<class A>
concept Anntt = requires(A a) { AnnotateTest(a); };

// ToTuple
template<class ... as>
constexpr auto ToTupleHelper(Annotate<as...>) {
    return TTuple<as...>{};
}

template<Anntt Ann>
using ToTuple = decltype(ToTupleHelper(Ann{}));
// end ToTuple

template<class T>
using ToFStat = FStat<T>;

// for groupby
template<class T1, class T2>
struct FirstIsAnnotation {
    static constexpr bool Value = Anntt<typename T1::Type>;
};

// for fold
template<class Stat1, class Stat2>
struct Join {};

template<class T1, class ... as1, class ... as, class ... as2>
struct Join<FStat<T1, as1...>, FStat<Annotate<as...>, as2...>> {
    using Type = FStat<Annotate<as...>, as1..., as..., as2...>;
};

template<class T1, class ... as1, class T2, class ... as2>
struct Join<FStat<T1, as1...>, FStat<T2, as2...>> {
    using Type = FStat<T2, as1..., as2...>;
};


template <class T>
struct Describe {
    using Types = decltype(asTTuple<T>());
    using Fields = Map<BindFold<Join, FStat<Annotate<>>>::template F, GroupBy<FirstIsAnnotation, Map<ToFStat, Types>>>;

    static constexpr size_t num_fields = Size<Fields>;

    template <size_t I>
    using Field = Get<I, Fields>;
};


// чтобы соблюсти интерфейс для функций, работающих с TTuple
template<class A, class B>
struct Same {
    static constexpr bool Value = std::is_same_v<A, B>;
};

template<template <typename, typename> typename G, class Arg1>
struct Bind {
    template<class Arg2>
    using F = G<Arg1, Arg2>;
};


template<template <typename...> typename Tpl, class Substituted>
struct SameTemplate {
    static constexpr bool Value = false;
};

template<template <typename...> typename Tpl, template<typename...> typename Substituted, class ... Args>
struct SameTemplate<Tpl, Substituted<Args...>> {
    static constexpr bool Value = false;
};

template<template<typename...> typename Substituted, class ... Args>
struct SameTemplate<Substituted, Substituted<Args...>> {
    static constexpr bool Value = true;
};

template<template <template <typename...> typename, class> typename G, template <typename...> typename Arg1>
struct BindTemplate {
    template<class Arg2>
    using F = G<Arg1, Arg2>;
};


template<class T, class ... annotations>
struct FStat {
    using Type = T;
    using Annotations = Annotate<annotations...>;

    template <template <class...> class AnnotationTemplate>
    static constexpr bool has_annotation_template = !EmptyTT<Filter<
            BindTemplate<SameTemplate, AnnotationTemplate>::template F,
            ToTuple<Annotations>
        >>;

    template <class Annotation>
    static constexpr bool has_annotation_class = !EmptyTT<Filter<
            Bind<Same, Annotation>::template F,
                    ToTuple<Annotations>
        >>;

    template <template <class...> class AnnotationTemplate>
            requires has_annotation_template<AnnotationTemplate>
    using FindAnnotation = Head<
            Filter<
                BindTemplate<SameTemplate, AnnotationTemplate>::template F,
                ToTuple<Annotations>
            >
        >;
};

// чтобы сократить число инстанцирований во время работы с начальным списком FStat
template<class ... As, class ... annotations>
struct FStat<Annotate<As...>, annotations...> {
    using Type = Annotate<As...>;
    using Annotations = Annotate<annotations...>;
};

#endif //MIPT_METAPROGRAMMING_REFLECT_HPP
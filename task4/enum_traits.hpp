#ifndef MIPT_METAPROGRAMMING_ENUM_TRAITS_HPP
#define MIPT_METAPROGRAMMING_ENUM_TRAITS_HPP

#include <algorithm>
#include <array>
#include <limits>
#include <ranges>
#include <string_view>
#include <type_traits>
#include <utility>

namespace details {

// clang
//std::string_view details::GetName() [Enum = Fruit, enum_obj = APPLE]
//std::string_view details::GetName() [Enum = Fruit, enum_obj = 3]
//std::string_view details::GetName() [Enum = Shape, enum_obj = Shape::SQUARE]
//std::string_view details::GetName() [Enum = Shape, enum_obj = 3]

// gcc
//constexpr std::string_view details::GetName() [with Enum = Fruit; Enum enum_obj = APPLE; std::string_view = std::basic_string_view<char>]
//constexpr std::string_view details::GetName() [with Enum = Fruit; Enum enum_obj = (Fruit)3; std::string_view = std::basic_string_view<char>]
//constexpr std::string_view details::GetName() [with Enum = Shape; Enum enum_obj = Shape::SQUARE; std::string_view = std::basic_string_view<char>]
//constexpr std::string_view details::GetName() [with Enum = Shape; Enum enum_obj = (Shape)3; std::string_view = std::basic_string_view<char>]

constexpr std::string_view START = "enum_obj = ";
constexpr std::string_view STOP = "];";
constexpr std::string_view DOUBLE_COLON = "::";

template<class Enum, Enum enum_obj>
requires std::is_enum_v<Enum>
constexpr std::string_view GetName() {
    constexpr std::string_view func{__PRETTY_FUNCTION__};

    constexpr size_t full_name_pos = func.find(START) + START.size();
    constexpr size_t full_name_length = func.substr(full_name_pos, func.size() - full_name_pos).find_first_of(STOP);
    constexpr std::string_view full_name = func.substr(full_name_pos, full_name_length);

    // remove type name of scoped enum
    constexpr size_t colon_pos = full_name.find(DOUBLE_COLON);
    size_t name_pos = 0;
    size_t name_length = full_name.size();
    if constexpr(colon_pos != std::string_view::npos) {
        name_pos = colon_pos + DOUBLE_COLON.size();
        name_length = full_name.size() - name_pos;
    }

    return full_name.substr(name_pos, name_length);
}

template<char c>
constexpr bool IsDigitOrMinusOrRoundBracket() {
    if constexpr((c >= '0' && c <= '9') || c == '-' || c == '(') {
        return true;
    }
    return false;
}

template<class Enum, long long i>
constexpr bool TrueEnum() {
    return !IsDigitOrMinusOrRoundBracket<GetName<Enum, Enum{i}>()[0]>();
}

template<class Enum, size_t N, bool trueEnum>
// trueEnum is used when N == 1
struct EnumMeta {
    using EnumType = std::underlying_type_t<Enum>;
    std::array<long long, N> values;
    size_t size;

    template<bool otherTrue>
    constexpr EnumMeta<Enum, N + 1, trueEnum> operator+(const EnumMeta<Enum, 1, otherTrue>& other) const {
        EnumMeta<Enum, N + 1, trueEnum> result;
        std::ranges::copy(values.begin(), values.begin() + size, result.values.begin());
        result.size = size;

        if constexpr(otherTrue) {
            std::ranges::copy(other.values.begin(), other.values.end(), result.values.begin() + size);
            result.size++;
        }

        return result;
    }
};


template<class Enum, size_t MAXN>
// Enum's underlying type is signed
struct EnumValues {
    using EnumType = std::underlying_type_t<Enum>;
    // нужно для случая signed char, где диапазон значений: [-128, 127]
    static constexpr long long maxVal = MAXN < std::numeric_limits<EnumType>::max() ? MAXN :
            std::numeric_limits<EnumType>::max();
    static constexpr long long minVal = -static_cast<long long>(MAXN) > std::numeric_limits<EnumType>::min() ?
            -static_cast<long long>(MAXN) :
            std::numeric_limits<EnumType>::min();
    static constexpr size_t SIZE = static_cast<size_t>(maxVal - minVal + 1);

    template<long long ... ints>
    using Sequence = std::integer_sequence<long long, ints...>;

    using Range = std::make_integer_sequence<long long, SIZE>;

    // implementation
    template<long long ... ints>
    static constexpr auto AdjustHelper(Sequence<ints...>) {
        return Sequence<(ints + minVal)...>{};
    }
    template<class Seq>
    using Adjust = decltype (AdjustHelper(Seq{}) );

    static constexpr size_t size = []<long long ... ints>(Sequence<ints...>) {
        return ((!TrueEnum<Enum, ints>() ? 1 : 0) +...);
    }(Adjust<Range>{});


    template<EnumMeta ... metas>
    struct SequenceMeta {};

    template<long long ... ints>
    static constexpr auto SequenceMetaHelper(Sequence<ints...>) {
        return SequenceMeta<EnumMeta<Enum, 1, TrueEnum<Enum, ints>()>{{ints}, 0}...>{};
    }

    template<class Seq>
    using MetaRange = decltype(SequenceMetaHelper(Seq{}));

    static constexpr EnumMeta<Enum, SIZE, false> meta = []<EnumMeta ... metas>(SequenceMeta<metas...>) {
        return (EnumMeta<Enum, 0, false>{{}, 0}+ ... +(metas));
    }(MetaRange<Adjust<Range>>{});

    // мапа имен для всех возможных integral values -- она не хотела класться массивом в EnumMeta
    static constexpr std::array<std::string_view, SIZE> names_map = []<long long ... ints>(Sequence<ints...>) {
        return std::array<std::string_view, SIZE>{GetName<Enum, Enum{ints}>()...};
    }(Adjust<Range>{});

    static constexpr std::string_view NameAt(size_t i) {
        return names_map[meta.values[i] - minVal];
    }
};

template<class Enum, size_t MAXN>
requires std::is_unsigned_v<std::underlying_type_t<Enum>> // unsigned
struct EnumValues<Enum, MAXN> {
    // to prettify code
    using EnumType = std::underlying_type_t<Enum>;
    static constexpr long long MAXVal = MAXN < std::numeric_limits<EnumType>::max() ? MAXN :
            std::numeric_limits<EnumType>::max();
    static constexpr size_t SIZE = MAXVal + 1;

    template<long long ... ints>
    using Sequence = std::integer_sequence<long long, ints...>;

    using Range = std::make_integer_sequence<long long, SIZE>;

    // implementation
    static constexpr size_t size = []<long long ... ints>(Sequence<ints...>) {
        return ((!TrueEnum<Enum, ints>() ? 1 : 0) +...);
    }(Range{});


    template<EnumMeta ... metas>
    struct SequenceMeta {
    };

    template<long long ... ints>
    static constexpr auto SequenceMetaHelper(Sequence<ints...>) {
        return SequenceMeta<EnumMeta<Enum, 1, TrueEnum<Enum, ints>()>{{ints}, 0}...>{};
    }

    template<class Seq>
    using MetaRange = decltype(SequenceMetaHelper(Seq{}));

    static constexpr EnumMeta<Enum, SIZE, false> meta = []<EnumMeta ... metas>(SequenceMeta<metas...>) {
        return (EnumMeta<Enum, 0, false>{{}, 0}+ ... +(metas));
    }(MetaRange<Range>{});

    // мапа имен для всех возможных integral values -- она не хотела класться массивом в EnumMeta
    static constexpr std::array<std::string_view, SIZE> names_map = []<long long ... ints>(Sequence<ints...>) {
        return std::array<std::string_view, SIZE>{GetName<Enum, Enum{ints}>()...};
    }(Range{});

    static constexpr std::string_view NameAt(size_t i) {
        return names_map[meta.values[i]];
    }
};

} // namespace details

template <class Enum, size_t MAXN=512>
requires std::is_enum_v<Enum>
struct EnumeratorTraits {
    using ThisEnumValues = details::EnumValues<Enum, MAXN>;
    using EnumType = std::underlying_type_t<Enum>;

    static constexpr size_t size() noexcept {
        return ThisEnumValues::meta.size;
    }

    static constexpr Enum at(size_t i) noexcept {
        return Enum{ThisEnumValues::meta.values[i]};
    }

    static constexpr std::string_view nameAt(size_t i) noexcept {
        return ThisEnumValues::NameAt(i);
    }
};

#endif //MIPT_METAPROGRAMMING_ENUM_TRAITS_HPP

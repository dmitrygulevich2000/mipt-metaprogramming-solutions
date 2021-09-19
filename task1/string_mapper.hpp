#ifndef MIPT_METAPROGRAMMING_SOLUTIONS_STRING_MAPPER_HPP
#define MIPT_METAPROGRAMMING_SOLUTIONS_STRING_MAPPER_HPP

#include <algorithm>

// String
template<size_t max_length>
class String {
public:
    char data[max_length]{};
    size_t length = 0;

    constexpr String(const char* string, std::size_t length): length(length) {
        std::ranges::copy(string, string + length, data);
    }

    constexpr operator std::string_view() const {
        return std::string_view(data, length);
    }
};

constexpr String<256> operator "" _cstr(const char* s, std::size_t l) {
    return {s, l};
}
// end String

// Mapping
template <class From, auto target>
struct Mapping {
    using from = From;
//    static const auto target_obj{target};
};

template <class Base, class Target, class ... Mappings>
struct ClassMapper {
    static std::optional<Target> map(const Base& object) {
        return std::nullopt;
    }
};

template <class Base, class Target, class M, class ... SubsMappings>
struct ClassMapper<Base, Target, M, SubsMappings ...> {
    static std::optional<Target> map(const Base& object) {
        try {
            dynamic_cast<const M::from&>(object);
            return []<class From, Target target> (Mapping<From, target>) {return target;} (M{});;

        } catch (const std::bad_cast&) {
            return ClassMapper<Base, Target, SubsMappings...>::map(object);
        }
    }
};
// end Mapping

#endif //MIPT_METAPROGRAMMING_SOLUTIONS_STRING_MAPPER_HPP

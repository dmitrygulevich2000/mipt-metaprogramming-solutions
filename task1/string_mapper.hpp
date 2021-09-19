#ifndef MIPT_METAPROGRAMMING_SOLUTIONS_STRING_MAPPER_HPP
#define MIPT_METAPROGRAMMING_SOLUTIONS_STRING_MAPPER_HPP

#include <algorithm>
#include <array>

// String
template<size_t max_length>
class String {
public:
    std::array<char, max_length> data;
    size_t length = 0;

    constexpr String(const char* string, std::size_t length): length(length) {
        std::ranges::copy(string, string + length, data.begin());
    }

    constexpr operator std::string_view() const {
        return std::string_view(data.begin(), data.begin() + length);
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
    static constexpr auto target_obj{target};
};

template <class Base, class Target, class ... Mappings>
struct ClassMapper {
    static std::optional<Target> map(const Base& object) {
        return std::nullopt;
    }
};

template <class Base, class Target, class HeadMapping, class ... SubsMappings>
struct ClassMapper<Base, Target, HeadMapping, SubsMappings ...> {
    static std::optional<Target> map(const Base& object) {
        auto casted = dynamic_cast<const typename HeadMapping::from*>(&object);
        if (casted) {
            return HeadMapping::target_obj;
        } else {
            return ClassMapper<Base, Target, SubsMappings...>::map(object);
        }
    }
};
// end Mapping

#endif //MIPT_METAPROGRAMMING_SOLUTIONS_STRING_MAPPER_HPP

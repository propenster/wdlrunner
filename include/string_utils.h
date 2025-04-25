// utility_functions.hpp
#pragma once

#include <string>
#include <cctype>
#include <algorithm>
#include <string_view>

namespace util
{
    // Convert a string to lowercase
    inline std::string to_lowercase(std::string_view input)
    {
        std::string result;
        result.reserve(input.size());
        for (const auto &c : input)
        {
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return result;
    }
    inline bool is_non_primitive_member_type(std::string_view type) noexcept
    {
        return to_lowercase(type) == "input" ||
               to_lowercase(type) == "output" ||
               to_lowercase(type) == "runtime" ||
               to_lowercase(type) == "meta";
    }
    inline bool array_contains_char(char chr) noexcept
    {
        const std::array<char, 12> arr = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_', '.'};
        return std::find(arr.begin(), arr.end(), chr) != arr.end();
    }

}
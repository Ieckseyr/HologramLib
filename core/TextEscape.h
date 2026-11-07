#pragma once

// 把任意文本安全地嵌进 JSON / SNBT 字符串字面量。
// 手写 std::format 拼 JSON 时，名字里出现 " \ 控制字符会直接产出非法文档。

#include <cstdio>
#include <string>
#include <string_view>

namespace debugshape_export::core::text {

inline std::string jsonEscape(std::string_view input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (unsigned char const ch : input) {
        switch (ch) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (ch < 0x20) {
                char buffer[8];
                std::snprintf(buffer, sizeof(buffer), "\\u%04x", ch);
                out += buffer;
            } else {
                out += static_cast<char>(ch);
            }
            break;
        }
    }
    return out;
}

// SNBT 字符串转义（规则同 JSON，不包引号）。
inline std::string snbtEscape(std::string_view input) {
    return jsonEscape(input);
}

} // namespace debugshape_export::core::text

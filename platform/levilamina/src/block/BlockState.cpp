#include "BlockState.h"

#include <mc/deps/core/string/HashedString.h>

#include <cctype>
#include <string>

namespace debugshape_export::blockstate {

std::string trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.pop_back();
    return s;
}

std::string unquote(std::string s) {
    if (s.size() >= 2 && (s.front() == '"' || s.front() == '\'') && s.back() == s.front()) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

// 解析 "minecraft:oak_stairs[waterlogged=true,upside_down_bit=false]" 这种状态串
bool parseBlockState(
    std::string const&                       text,
    std::string&                             outName,
    ::Block::BlockStatesType&                outStates
) {
    auto const        open  = text.find('[');
    std::string const name  = trim(open == std::string::npos ? text : text.substr(0, open));
    if (name.empty())
        return false;
    outName = name;

    if (open == std::string::npos)
        return true;

    auto const close = text.rfind(']');
    if (close == std::string::npos || close <= open)
        return false;

    std::string body = text.substr(open + 1, close - open - 1);
    std::vector<std::string> parts;
    std::string              current;
    bool                     inQuote = false;
    char                     quoteChar = 0;
    for (char ch : body) {
        if (inQuote) {
            current += ch;
            if (ch == quoteChar)
                inQuote = false;
            continue;
        }
        if (ch == '"' || ch == '\'') {
            inQuote   = true;
            quoteChar = ch;
            current += ch;
            continue;
        }
        if (ch == ',') {
            parts.push_back(current);
            current.clear();
            continue;
        }
        current += ch;
    }
    if (!current.empty())
        parts.push_back(current);

    for (auto const& raw : parts) {
        auto const part = trim(raw);
        if (part.empty())
            continue;
        auto const eq = part.find('=');
        if (eq == std::string::npos)
            return false;
        auto const key = trim(part.substr(0, eq));
        auto const val = unquote(trim(part.substr(eq + 1)));
        if (key.empty())
            return false;

        if (val == "true" || val == "false") {
            outStates.emplace_back(key, val == "true");
            continue;
        }

        char* end      = nullptr;
        long  intValue = std::strtol(val.c_str(), &end, 10);
        if (end != nullptr && *end == '\0' && !val.empty()) {
            outStates.emplace_back(key, static_cast<int>(intValue));
            continue;
        }

        double floatValue = std::strtod(val.c_str(), &end);
        if (end != nullptr && *end == '\0' && !val.empty()) {
            outStates.emplace_back(key, static_cast<float>(floatValue));
            continue;
        }

        outStates.emplace_back(key, val);
    }
    return true;
}

bool resolveNetworkId(std::string const& stateText, std::uint32_t& outId) {
    std::string              name;
    ::Block::BlockStatesType states;
    if (!parseBlockState(stateText, name, states)) {
        return false;
    }
    auto block = ::Block::tryGetFromRegistry(::HashedString(name), states);
    if (!block) {
        return false;
    }
    outId = static_cast<std::uint32_t>(block->mNetworkId);
    return true;
}

} // namespace debugshape_export::blockstate

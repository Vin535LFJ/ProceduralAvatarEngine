#include "ht_json.h"

#include "ht_common.h"

#include <cstdlib>

namespace pae::ht {

namespace {

struct Parser {
    const std::string& s;
    size_t i = 0;
    std::string error;

    explicit Parser(const std::string& text) : s(text) {}

    void skipSpace() {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) ++i;
    }

    bool fail(const std::string& message) {
        if (error.empty()) error = fmt("offset %llu: %s", static_cast<unsigned long long>(i), message.c_str());
        return false;
    }

    bool literal(const char* word) {
        const size_t n = std::char_traits<char>::length(word);
        if (s.compare(i, n, word) != 0) return false;
        i += n;
        return true;
    }

    bool parseString(std::string& out) {
        if (i >= s.size() || s[i] != '"') return fail("expected string");
        ++i;
        while (i < s.size()) {
            const char c = s[i++];
            if (c == '"') return true;
            if (c == '\\') {
                if (i >= s.size()) return fail("bad escape");
                const char e = s[i++];
                switch (e) {
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'n': out += '\n'; break;
                    case 'r': out += '\r'; break;
                    case 't': out += '\t'; break;
                    case 'u': {
                        if (i + 4 > s.size()) return fail("bad \\u escape");
                        const unsigned code = static_cast<unsigned>(std::strtoul(s.substr(i, 4).c_str(), nullptr, 16));
                        i += 4;
                        // UTF-8 encoding (surrogates are not combined).
                        if (code < 0x80) out += static_cast<char>(code);
                        else if (code < 0x800) {
                            out += static_cast<char>(0xC0 | (code >> 6));
                            out += static_cast<char>(0x80 | (code & 0x3F));
                        } else {
                            out += static_cast<char>(0xE0 | (code >> 12));
                            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                            out += static_cast<char>(0x80 | (code & 0x3F));
                        }
                        break;
                    }
                    default: return fail("bad escape");
                }
            } else {
                out += c;
            }
        }
        return fail("unterminated string");
    }

    bool parseValue(JsonValue& out, int depth) {
        if (depth > 200) return fail("nesting too deep");
        skipSpace();
        if (i >= s.size()) return fail("unexpected end");
        const char c = s[i];
        if (c == '{') {
            ++i;
            out.type = JsonValue::Type::Object;
            skipSpace();
            if (i < s.size() && s[i] == '}') { ++i; return true; }
            while (true) {
                skipSpace();
                std::string key;
                if (!parseString(key)) return false;
                skipSpace();
                if (i >= s.size() || s[i] != ':') return fail("expected ':'");
                ++i;
                JsonValue value;
                if (!parseValue(value, depth + 1)) return false;
                out.members.push_back({key, value});
                skipSpace();
                if (i < s.size() && s[i] == ',') { ++i; continue; }
                if (i < s.size() && s[i] == '}') { ++i; return true; }
                return fail("expected ',' or '}'");
            }
        }
        if (c == '[') {
            ++i;
            out.type = JsonValue::Type::Array;
            skipSpace();
            if (i < s.size() && s[i] == ']') { ++i; return true; }
            while (true) {
                JsonValue value;
                if (!parseValue(value, depth + 1)) return false;
                out.items.push_back(value);
                skipSpace();
                if (i < s.size() && s[i] == ',') { ++i; continue; }
                if (i < s.size() && s[i] == ']') { ++i; return true; }
                return fail("expected ',' or ']'");
            }
        }
        if (c == '"') {
            out.type = JsonValue::Type::String;
            return parseString(out.text);
        }
        if (literal("true")) { out.type = JsonValue::Type::Bool; out.boolean = true; return true; }
        if (literal("false")) { out.type = JsonValue::Type::Bool; out.boolean = false; return true; }
        if (literal("null")) { out.type = JsonValue::Type::Null; return true; }
        // Number
        {
            char* end = nullptr;
            const double v = std::strtod(s.c_str() + i, &end);
            if (end == s.c_str() + i) return fail("unexpected character");
            out.type = JsonValue::Type::Number;
            out.number = v;
            i = static_cast<size_t>(end - s.c_str());
            return true;
        }
    }
};

} // namespace

const JsonValue* JsonValue::find(const std::string& key) const {
    if (type != Type::Object) return nullptr;
    for (const auto& m : members)
        if (m.first == key) return &m.second;
    return nullptr;
}

bool parseJson(const std::string& text, JsonValue& out, std::string& error) {
    Parser p(text);
    out = JsonValue{};
    if (!p.parseValue(out, 0)) {
        error = p.error;
        return false;
    }
    p.skipSpace();
    if (p.i != text.size()) {
        error = fmt("trailing content at offset %llu", static_cast<unsigned long long>(p.i));
        return false;
    }
    return true;
}

} // namespace pae::ht

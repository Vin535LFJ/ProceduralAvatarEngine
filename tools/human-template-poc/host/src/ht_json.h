#pragma once

// Minimal JSON reader (objects, arrays, strings, numbers, booleans, null).
// Used for the MakeHuman rig (.mhskel) and weights (.mhw) definitions.

#include <string>
#include <utility>
#include <vector>

namespace pae::ht {

struct JsonValue {
    enum class Type { Null, Bool, Number, String, Array, Object };
    Type type = Type::Null;
    bool boolean = false;
    double number = 0.0;
    std::string text;
    std::vector<JsonValue> items;
    std::vector<std::pair<std::string, JsonValue>> members;

    const JsonValue* find(const std::string& key) const;
    bool isObject() const { return type == Type::Object; }
    bool isArray() const { return type == Type::Array; }
    bool isString() const { return type == Type::String; }
    bool isNumber() const { return type == Type::Number; }
};

bool parseJson(const std::string& text, JsonValue& out, std::string& error);

} // namespace pae::ht

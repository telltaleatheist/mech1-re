// Strict reading of our JSON files (the story's record files, src/Import;
// the rules, src/Rules): the shape is checked, and every error names the file.
// Error is the caller's exception type, constructible from a string.
#pragma once

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Json.h"

namespace mw1o::support {

template <class Error>
std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw Error("can't read " + path.string());
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

template <class Error>
class StrictReader {
public:
    explicit StrictReader(std::filesystem::path path) : path_(std::move(path)) {}

    Json parse() {
        try {
            return parse_json(read_text_file<Error>(path_));
        } catch (const JsonError& e) {
            fail(e.what());
        }
    }
    [[noreturn]] void fail(const std::string& why) const { throw Error(path_.string() + ": " + why); }

    const Json& member(const Json& object, std::string_view key) const {
        if (!object.is_object()) fail("expected an object");
        const Json* found = object.find(key);
        if (!found) fail("no \"" + std::string(key) + "\"");
        return *found;
    }
    const Json::Array& array(const Json& object, std::string_view key) const {
        const Json& v = member(object, key);
        if (!v.is_array()) fail("\"" + std::string(key) + "\" is not an array");
        return v.array();
    }
    // An array member of exactly `count` entries.
    const Json::Array& array(const Json& object, std::string_view key, std::size_t count) const {
        const Json::Array& a = array(object, key);
        if (a.size() != count)
            fail("\"" + std::string(key) + "\" has " + std::to_string(a.size()) + " entries, not " +
                 std::to_string(count));
        return a;
    }
    // Refuses any member of `object` not named in `keys`.
    void only(const Json& object, std::initializer_list<std::string_view> keys) const {
        only(object, std::span<const std::string_view>(keys.begin(), keys.size()));
    }
    void only(const Json& object, std::span<const std::string_view> keys) const {
        if (!object.is_object()) fail("expected an object");
        for (const auto& [key, value] : object.object()) {
            bool known = false;
            for (std::string_view k : keys) known = known || k == key;
            if (!known) fail("unknown key \"" + key + "\"");
        }
    }
    // The members of an object keyed by exactly `names` (each present, nothing
    // else), in the order of `names`.
    std::vector<const Json*> keyed(const Json& object, std::string_view key,
                                   std::span<const std::string_view> names) const {
        const Json& v = member(object, key);
        if (!v.is_object()) fail("\"" + std::string(key) + "\" is not an object");
        only(v, names);
        std::vector<const Json*> out;
        for (std::string_view name : names) out.push_back(&member(v, name));
        return out;
    }
    double whole(const Json& v, std::string_view what, double maximum) const {
        if (!v.is_number() || v.number() < 0 || v.number() > maximum || std::floor(v.number()) != v.number())
            fail(std::string(what) + " is not a whole number from 0 to " + std::to_string(static_cast<long>(maximum)));
        return v.number();
    }
    // A value already in hand, as a byte or a word.
    std::uint8_t byte(const Json& v, std::string_view what) const {
        return static_cast<std::uint8_t>(whole(v, what, 255));
    }
    std::uint16_t word(const Json& v, std::string_view what) const {
        return static_cast<std::uint16_t>(whole(v, what, 65535));
    }
    // A member, as a byte or a word.
    std::uint8_t u8(const Json& object, std::string_view key) const { return byte(member(object, key), key); }
    std::uint16_t u16(const Json& object, std::string_view key) const { return word(member(object, key), key); }
    std::string text(const Json& object, std::string_view key) const {
        const Json& v = member(object, key);
        if (!v.is_string() || v.string().empty()) fail("\"" + std::string(key) + "\" is not a text id");
        return v.string();
    }
    std::vector<int> id_list(const Json& list, std::size_t count) const {
        if (!list.is_array()) fail("expected a list of ids");
        std::vector<int> out;
        for (const Json& v : list.array()) {
            const int id = static_cast<int>(whole(v, "an id", 65535));
            if (id < 1 || static_cast<std::size_t>(id) > count) fail("the id " + std::to_string(id) + " names no record");
            out.push_back(id);
        }
        return out;
    }

private:
    std::filesystem::path path_;
};

}  // namespace mw1o::support

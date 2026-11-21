// The story's record files are read strictly: the shape is checked, and every
// error names the file (Records.cpp, Events.cpp). Internal to src/Import.
#pragma once

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include "../Support/Json.h"
#include "Story.h"

namespace mw1o::import::detail {

namespace fs = std::filesystem;
using support::Json;

inline std::string read_text_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw ImportError("can't read " + path.string());
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

// Strict reading of one record file: the shape is checked, and every error
// names the file.
class Reader {
public:
    explicit Reader(fs::path path) : path_(std::move(path)) {}

    Json parse() {
        try {
            return support::parse_json(read_text_file(path_));
        } catch (const support::JsonError& e) {
            fail(e.what());
        }
    }
    [[noreturn]] void fail(const std::string& why) const { throw ImportError(path_.string() + ": " + why); }

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
    double whole(const Json& v, std::string_view what, double maximum) const {
        if (!v.is_number() || v.number() < 0 || v.number() > maximum || std::floor(v.number()) != v.number())
            fail(std::string(what) + " is not a whole number from 0 to " + std::to_string(static_cast<long>(maximum)));
        return v.number();
    }
    std::uint8_t u8(const Json& object, std::string_view key) const {
        return static_cast<std::uint8_t>(whole(member(object, key), key, 255));
    }
    std::uint16_t u16(const Json& object, std::string_view key) const {
        return static_cast<std::uint16_t>(whole(member(object, key), key, 65535));
    }
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
    fs::path path_;
};

}  // namespace mw1o::import::detail

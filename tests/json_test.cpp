// src/Support/Json: every byte survives a round trip, the strict rules refuse
// what they should and name the line, and deep nesting stops at the limit.
// Plain asserts, no framework: one executable, exit 0 when everything holds.
#include "../src/Support/Json.h"

#include <cstdio>
#include <string>

using mw1o::support::Json;
using mw1o::support::JsonError;
using mw1o::support::json_quote;
using mw1o::support::kJsonMaxDepth;
using mw1o::support::parse_json;

namespace {

int g_failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        ++g_failures;
        std::printf("FAIL %s\n", what.c_str());
    }
}

// `text` must be refused, and the message must name `line`.
void refused(const std::string& text, int line, const std::string& what) {
    try {
        parse_json(text);
        check(false, what + ": accepted");
    } catch (const JsonError& e) {
        const std::string want = "line " + std::to_string(line) + ", column ";
        check(std::string(e.what()).find(want) != std::string::npos,
              what + ": message \"" + e.what() + "\" doesn't name " + want);
    }
}

void accepted(const std::string& text, const std::string& what) {
    try {
        parse_json(text);
    } catch (const JsonError& e) {
        check(false, what + ": " + e.what());
    }
}

void every_byte_round_trips() {
    std::string all;
    for (int b = 0; b < 256; ++b) all += static_cast<char>(b);
    const std::string quoted = json_quote(all);
    for (const char c : quoted) {
        const auto u = static_cast<unsigned char>(c);
        check(u >= 0x20 && u < 0x7f, "json_quote output is printable ASCII");
    }
    const Json back = parse_json("{\"t\":" + quoted + "}");
    check(back.find("t") && back.find("t")->string() == all, "bytes 00..FF together");
    for (int b = 0; b < 256; ++b) {
        const std::string one(1, static_cast<char>(b));
        check(parse_json(json_quote(one)).string() == one, "byte " + std::to_string(b) + " alone");
    }
}

void the_strict_rules() {
    refused("{\"a\":1,\n\"a\":2}", 2, "duplicate key");
    refused("{\"a\":1,\n}", 2, "trailing comma in an object");
    refused("[1,\n2,\n]", 3, "trailing comma in an array");
    refused("[,1]", 1, "leading comma");
    refused("\"a\\qb\"", 1, "unknown escape");
    refused("\"\\u00g0\"", 1, "\\u with a non-hex digit");
    refused("\"\\u 0ff\"", 1, "\\u with a space");
    refused("\"\\u-001\"", 1, "\\u with a sign");
    refused("\"\\u+0ff\"", 1, "\\u with a plus");
    refused("\"\\u00f\"", 1, "\\u with three digits");
    refused("\"\\u0100\"", 1, "\\u above 00ff");
    refused("\"a\nb\"", 1, "raw newline in a string");
    refused("\"a\tb\"", 1, "raw tab in a string");
    refused("\"abc", 1, "unfinished string");
    refused("\n\n\n{\"a\" 1}", 4, "missing colon");
    refused("01", 1, "leading zero");
    refused("-01", 1, "leading zero after minus");
    refused("1.", 1, "dot without digits");
    refused(".5", 1, "no digit before the dot");
    refused("1e", 1, "exponent without digits");
    refused("+1", 1, "plus sign");
    refused("0x10", 1, "hex number");
    refused("1 2", 1, "two values");
    refused("tru", 1, "short word");
    refused("", 1, "empty text");
    refused("{\"a\":[1,2}", 1, "mismatched bracket");

    accepted("{\"a\":1,\"b\":[true,false,null],\"c\":\"\\u00ff\\n\\\"\"}", "a plain document");
    accepted("[0, -0, 1.5, -2e10, 3E+2, 4e-2, 123456789]", "numbers in JSON's grammar");
    check(parse_json("[1.5]").array()[0].number() == 1.5, "1.5 reads as 1.5 whatever the locale");
    check(parse_json("[-2e3]").array()[0].number() == -2000, "-2e3");
    check(parse_json("{\"k\":{\"k\":2}}").find("k")->find("k")->number() == 2, "the same key at two levels");
}

// Inception's message form: "JSON at line L, column C: why", the column in bytes.
void the_message() {
    const auto message = [](const std::string& text) {
        try {
            parse_json(text);
        } catch (const JsonError& e) {
            return std::string(e.what());
        }
        return std::string("accepted");
    };
    check(message("{\"a\":1,\n  \"a\":2}") == "JSON at line 2, column 3: duplicate key", "duplicate key at its key");
    check(message("[1,\n 012]") == "JSON at line 2, column 2: a number with a leading zero", "leading zero at the number");
    check(message("\"ab\tc\"") == "JSON at line 1, column 4: control character in a string (write it as \\u00XX)",
          "control character at itself");
    check(message("\"\\u00g0\"") == "JSON at line 1, column 4: \\u needs four hex digits", "\\u message");
    check(message(std::string(300, '[')) == "JSON at line 1, column 257: nested deeper than 256", "depth at the bracket");
}

void the_depth_limit() {
    const auto nested = [](std::size_t n) { return std::string(n, '[') + std::string(n, ']'); };
    accepted(nested(kJsonMaxDepth), "nesting at the limit");
    refused(nested(kJsonMaxDepth + 1), 1, "nesting past the limit");
    refused(std::string(100000, '['), 1, "a long run of [ (no stack overflow)");
    std::string objects;
    for (std::size_t i = 0; i <= kJsonMaxDepth; ++i) objects += "{\"a\":";
    objects += "1";
    for (std::size_t i = 0; i <= kJsonMaxDepth; ++i) objects += "}";
    refused(objects, 1, "objects past the limit");
}

}  // namespace

int main() {
    every_byte_round_trips();
    the_strict_rules();
    the_message();
    the_depth_limit();
    if (g_failures) {
        std::printf("json_test: %d failed\n", g_failures);
        return 1;
    }
    std::printf("json_test: all passed\n");
    return 0;
}

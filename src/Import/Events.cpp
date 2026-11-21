#include "Events.h"

#include <algorithm>

#include "../Support/Json.h"
#include "StrictReader.h"

namespace mw1o::import {

namespace fs = std::filesystem;
using detail::Reader;
using support::Json;
using support::json_quote;

namespace {

// The original's layout of the event data (MW_MAIN 1090), as
// Original/Memory/MW_MAIN_1090_EVENTS.cpp reads it.
constexpr std::uint16_t kDataStart = 0x5CF4, kDataEnd = 0x655C;
constexpr std::uint16_t kCaptionStart = 0x5CF4, kCaptionEnd = 0x5E1C;
constexpr std::uint16_t kListsStart = 0x6067, kEntriesStart = 0x6073, kEntriesTerminator = 0x611F;
constexpr std::uint16_t kRecordStart = 0x6121, kRecordEnd = 0x655C, kFixedEvent = 0x6353;
constexpr std::uint8_t kScriptEnd = 0xFF;
constexpr std::size_t kPoolSizes[] = {8, 3, 4, 3, 4, 3};  // 1090:8FDA, 8FE2, 8FE5, 8FE9, 8FEC, 8FF0
constexpr std::uint8_t kConditionSizes[10] = {1, 3, 1, 1, 2, 1, 0, 0, 0, 0};
constexpr std::uint8_t kActionSizes[10] = {0, 0, 6, 0, 4, 0, 0, 0, 1, 0};
constexpr std::uint8_t kChoices = 9;

class Decoder {
public:
    explicit Decoder(const PieceBytes& data) : data_(data.bytes) {
        if (data.offset != kDataStart || data_.size() != std::size_t(kDataEnd - kDataStart))
            throw ImportError("the import table's piece events.data is not 1090:5CF4..655C");
    }

    Events decode() {
        Events out;
        // The captions: a colour byte, then the text, back to back.
        std::uint16_t cursor = kCaptionStart;
        while (cursor < kCaptionEnd) {
            EventCaption caption;
            caption.address = cursor;
            caption.colour = byte(cursor++);
            const std::uint16_t end = nul(cursor, kCaptionEnd);
            caption.text = "event.caption." + std::to_string(out.captions.size() + 1);
            out.captions.push_back(caption);
            cursor = static_cast<std::uint16_t>(end + 1);
        }
        if (cursor != kCaptionEnd) fail("the captions overrun their area");
        captions_ = &out.captions;

        // The records, in the order the original's importer meets them: the
        // list entries, the fixed event, then each record's choice branches.
        for (std::uint16_t at = kEntriesStart; at <= kEntriesTerminator; at += 2) {
            const std::uint16_t offset = word(at);
            if ((at == kEntriesTerminator) != (offset == 0)) fail("the event list entries don't end with their one zero");
            out.list_entries.push_back(reference(offset));
        }
        for (std::uint16_t at = kListsStart; at < kEntriesStart; at += 2) {
            const std::uint16_t offset = word(at);
            if (!offset) {
                out.lists.push_back(std::nullopt);
                continue;
            }
            if (offset < kEntriesStart || offset > kEntriesTerminator || (offset - kEntriesStart) % 2)
                fail("an event list points outside the entries");
            out.lists.push_back(int((offset - kEntriesStart) / 2) + 1);
        }
        const auto fixed = reference(kFixedEvent);
        out.fixed_event = *fixed;
        for (std::size_t r = 0; r < offsets_.size(); ++r) {  // grows as branches are met
            EventRecord record{byte(offsets_[r]), byte(static_cast<std::uint16_t>(offsets_[r] + 1)), {}};
            std::uint16_t at = static_cast<std::uint16_t>(offsets_[r] + 2);
            bool action = false;
            while (byte(at) != kScriptEnd) {
                EventCommand command;
                command.action = action;
                command.opcode = byte(at++);
                if (command.opcode >= 10) fail("an event command's opcode is past 9");
                std::uint16_t size = action ? kActionSizes[command.opcode] : kConditionSizes[command.opcode];
                if (command.opcode == kChoices) {
                    const std::uint8_t count = byte(at);
                    if (count != 2 && count != 3) fail("a choice command has neither 2 nor 3 choices");
                    size = static_cast<std::uint16_t>(count * 4 + 1);
                    for (std::uint8_t i = 0; i < count; ++i) {
                        EventChoice choice;
                        choice.caption = caption_at(word(static_cast<std::uint16_t>(at + 1 + i * 2)));
                        choice.branch = reference(word(static_cast<std::uint16_t>(at + 1 + count * 2 + i * 2)));
                        command.choices.push_back(choice);
                    }
                } else if (!action) {
                    switch (command.opcode) {
                    case 0: command.random_threshold = byte(at); break;
                    case 1: command.date = date(at); break;
                    case 2: case 5: command.planet_selector = static_cast<std::int8_t>(byte(at)); break;
                    case 3: command.seen_event = byte(at); break;
                    case 4:
                        command.house_selector = byte(at);
                        command.threshold_half = static_cast<std::int8_t>(byte(static_cast<std::uint16_t>(at + 1)));
                        break;
                    default: break;
                    }
                } else {
                    switch (command.opcode) {
                    case 2:
                        command.window_start = date(at);
                        command.window_end = date(static_cast<std::uint16_t>(at + 3));
                        break;
                    case 4:
                        command.cash_delta = std::uint32_t(word(at)) | std::uint32_t(word(static_cast<std::uint16_t>(at + 2))) << 16;
                        break;
                    case 8: command.planet_selector = static_cast<std::int8_t>(byte(at)); break;
                    default: break;
                    }
                }
                if (at + size >= kRecordEnd) fail("an event script runs past the records");
                at = static_cast<std::uint16_t>(at + size);
                record.script.push_back(std::move(command));
                action = !action;
            }
            out.events.push_back(std::move(record));
        }
        return out;
    }

private:
    [[noreturn]] static void fail(const std::string& why) { throw ImportError("events.data: " + why); }
    std::uint8_t byte(std::uint16_t at) const {
        if (at < kDataStart || at >= kDataEnd) fail("a read outside 5CF4..655C");
        return static_cast<std::uint8_t>(data_[at - kDataStart]);
    }
    std::uint16_t word(std::uint16_t at) const {
        return static_cast<std::uint16_t>(byte(at) | byte(static_cast<std::uint16_t>(at + 1)) << 8);
    }
    EventDate date(std::uint16_t at) const {
        return {byte(at), byte(static_cast<std::uint16_t>(at + 1)), byte(static_cast<std::uint16_t>(at + 2))};
    }
    std::uint16_t nul(std::uint16_t at, std::uint16_t end) const {
        for (; at < end; ++at)
            if (!byte(at)) return at;
        fail("a caption has no NUL");
    }
    // A record reference: 0 is none; a new offset becomes the next record.
    std::optional<int> reference(std::uint16_t offset) {
        if (!offset) return std::nullopt;
        const auto it = std::find(offsets_.begin(), offsets_.end(), offset);
        if (it != offsets_.end()) return int(it - offsets_.begin()) + 1;
        if (offset < kRecordStart || offset + 2 >= kRecordEnd) fail("an event reference points outside the records");
        offsets_.push_back(offset);
        return int(offsets_.size());
    }
    int caption_at(std::uint16_t offset) const {
        for (std::size_t i = 0; i < captions_->size(); ++i)
            if ((*captions_)[i].address == offset) return int(i) + 1;
        fail("a choice names no caption");
    }

    const std::string& data_;
    std::vector<std::uint16_t> offsets_;
    const std::vector<EventCaption>* captions_ = nullptr;
};

std::string reference_json(const std::optional<int>& r) { return r ? std::to_string(*r) : "null"; }
std::string date_json(const EventDate& d) {
    return "{\"month\": " + std::to_string(d.month) + ", \"submonth\": " + std::to_string(d.submonth) +
           ", \"year\": " + std::to_string(d.year) + "}";
}

std::string command_json(const EventCommand& c) {
    std::string out = std::string("{\"kind\": \"") + (c.action ? "action" : "condition") +
                      "\", \"opcode\": " + std::to_string(c.opcode);
    if (c.opcode == kChoices) {
        out += ", \"choices\": [";
        for (std::size_t i = 0; i < c.choices.size(); ++i)
            out += std::string(i ? ", " : "") + "{\"caption\": " + std::to_string(c.choices[i].caption) +
                   ", \"branch\": " + reference_json(c.choices[i].branch) + "}";
        out += "]";
    } else if (!c.action) {
        switch (c.opcode) {
        case 0: out += ", \"random_threshold\": " + std::to_string(c.random_threshold); break;
        case 1: out += ", \"date\": " + date_json(c.date); break;
        case 2: case 5: out += ", \"planet_selector\": " + std::to_string(c.planet_selector); break;
        case 3: out += ", \"seen_event\": " + std::to_string(c.seen_event); break;
        case 4:
            out += ", \"house_selector\": " + std::to_string(c.house_selector) +
                   ", \"threshold_half\": " + std::to_string(c.threshold_half);
            break;
        default: break;
        }
    } else {
        switch (c.opcode) {
        case 2: out += ", \"window\": {\"start\": " + date_json(c.window_start) + ", \"end\": " + date_json(c.window_end) + "}"; break;
        case 4: out += ", \"cash_delta\": " + std::to_string(c.cash_delta); break;
        case 8: out += ", \"planet_selector\": " + std::to_string(c.planet_selector); break;
        default: break;
        }
    }
    return out + "}";
}

// The reader's extras for events: signed bytes, references, dates.
struct EventReader : Reader {
    using Reader::Reader;
    std::int8_t s8(const Json& object, std::string_view key) const {
        const Json& v = member(object, key);
        if (!v.is_number() || v.number() < -128 || v.number() > 127 || std::floor(v.number()) != v.number())
            fail(std::string(key) + " is not a whole number from -128 to 127");
        return static_cast<std::int8_t>(v.number());
    }
    std::uint32_t u32(const Json& object, std::string_view key) const {
        return static_cast<std::uint32_t>(whole(member(object, key), key, 4294967295.0));
    }
    // A reference from 1 to `count`, or null when `nullable`.
    std::optional<int> ref(const Json& v, std::string_view what, std::size_t count, bool nullable) const {
        if (std::holds_alternative<std::nullptr_t>(v.value)) {
            if (!nullable) fail(std::string(what) + " can't be null");
            return std::nullopt;
        }
        const int n = static_cast<int>(whole(v, what, 65535));
        if (n < 1 || std::size_t(n) > count) fail(std::string(what) + " " + std::to_string(n) + " names nothing");
        return n;
    }
    EventDate date(const Json& v) const { return {u8(v, "month"), u8(v, "submonth"), u8(v, "year")}; }
};

}  // namespace

std::string events_json(const PieceMap& pieces) {
    const auto data = pieces.find("events.data");
    const auto pools = pieces.find("events.planet_pools");
    if (data == pieces.end() && pools == pieces.end())
        return "{\n  \"captions\": [],\n  \"events\": [],\n  \"list_entries\": [],\n  \"lists\": [],\n"
               "  \"fixed_event\": null,\n  \"planet_pools\": []\n}\n";
    if (data == pieces.end() || pools == pieces.end())
        throw ImportError("the import table has only one of events.data and events.planet_pools");
    Events events = Decoder(data->second).decode();

    std::string out = "{\n  \"captions\": [";
    for (std::size_t i = 0; i < events.captions.size(); ++i)
        out += std::string(i ? "," : "") + "\n    {\"colour\": " + std::to_string(events.captions[i].colour) +
               ", \"address\": " + std::to_string(events.captions[i].address) + ", \"text\": " +
               json_quote(events.captions[i].text) + "}";
    out += "\n  ],\n  \"events\": [";
    for (std::size_t e = 0; e < events.events.size(); ++e) {
        const EventRecord& r = events.events[e];
        out += std::string(e ? "," : "") + "\n    {\"id\": " + std::to_string(r.id) + ", \"class\": " +
               std::to_string(r.class_code) + ", \"script\": [";
        for (std::size_t c = 0; c < r.script.size(); ++c) out += std::string(c ? ",\n      " : "\n      ") + command_json(r.script[c]);
        out += std::string(r.script.empty() ? "" : "\n    ") + "]}";
    }
    out += "\n  ],\n  \"list_entries\": [";
    for (std::size_t i = 0; i < events.list_entries.size(); ++i)
        out += std::string(i ? ", " : "") + reference_json(events.list_entries[i]);
    out += "],\n  \"lists\": [";
    for (std::size_t i = 0; i < events.lists.size(); ++i) out += std::string(i ? ", " : "") + reference_json(events.lists[i]);
    out += "],\n  \"fixed_event\": " + std::to_string(events.fixed_event) + ",\n  \"planet_pools\": [";
    std::size_t sizes = 0;
    for (std::size_t size : kPoolSizes) sizes += size;
    if (pools->second.offset != 0x8FDA || pools->second.bytes.size() != sizes)
        throw ImportError("the import table's piece events.planet_pools is not 1090:8FDA..8FF3");
    std::size_t at = 0;
    for (std::size_t p = 0; p < std::size(kPoolSizes); ++p) {
        out += std::string(p ? ", " : "") + "[";
        for (std::size_t i = 0; i < kPoolSizes[p]; ++i, ++at)
            out += std::string(i ? ", " : "") + std::to_string(static_cast<std::uint8_t>(pools->second.bytes[at]));
        out += "]";
    }
    return out + "]\n}\n";
}

Events load_events(const fs::path& write_dir) {
    EventReader r(story_dir(write_dir) / "events.json");
    const Json file = r.parse();
    Events out;
    for (const Json& c : r.array(file, "captions"))
        out.captions.push_back({r.u8(c, "colour"), r.u16(c, "address"), r.text(c, "text")});
    const auto& events = r.array(file, "events");
    const auto& entries = r.array(file, "list_entries");
    for (const Json& e : events) {
        EventRecord record{r.u8(e, "id"), r.u8(e, "class"), {}};
        bool action = false;
        for (const Json& c : r.array(e, "script")) {
            EventCommand command;
            const Json& kind = r.member(c, "kind");
            if (!kind.is_string() || kind.string() != (action ? "action" : "condition"))
                r.fail(std::string("a script's commands must alternate condition and action; expected ") +
                       (action ? "an action" : "a condition"));
            command.action = action;
            command.opcode = r.u8(c, "opcode");
            if (command.opcode >= 10) r.fail("an opcode is past 9");
            if (command.opcode == kChoices) {
                const auto& choices = r.array(c, "choices");
                if (choices.size() != 2 && choices.size() != 3) r.fail("a choice command has neither 2 nor 3 choices");
                for (const Json& choice : choices)
                    command.choices.push_back({*r.ref(r.member(choice, "caption"), "caption", 1 << 16, false),
                                               r.ref(r.member(choice, "branch"), "branch", events.size(), true)});
            } else if (!action) {
                switch (command.opcode) {
                case 0: command.random_threshold = r.u8(c, "random_threshold"); break;
                case 1: command.date = r.date(r.member(c, "date")); break;
                case 2: case 5: command.planet_selector = r.s8(c, "planet_selector"); break;
                case 3: command.seen_event = r.u8(c, "seen_event"); break;
                case 4: command.house_selector = r.u8(c, "house_selector"); command.threshold_half = r.s8(c, "threshold_half"); break;
                default: break;
                }
            } else {
                switch (command.opcode) {
                case 2:
                    command.window_start = r.date(r.member(r.member(c, "window"), "start"));
                    command.window_end = r.date(r.member(r.member(c, "window"), "end"));
                    break;
                case 4: command.cash_delta = r.u32(c, "cash_delta"); break;
                case 8: command.planet_selector = r.s8(c, "planet_selector"); break;
                default: break;
                }
            }
            record.script.push_back(std::move(command));
            action = !action;
        }
        out.events.push_back(std::move(record));
    }
    for (const auto& event : out.events)
        for (const auto& command : event.script)
            for (const auto& choice : command.choices)
                if (std::size_t(choice.caption) > out.captions.size()) r.fail("a choice names no caption");
    for (const Json& e : entries) out.list_entries.push_back(r.ref(e, "a list entry", out.events.size(), true));
    for (const Json& l : r.array(file, "lists")) out.lists.push_back(r.ref(l, "a list", out.list_entries.size(), true));
    if (!out.events.empty()) out.fixed_event = *r.ref(r.member(file, "fixed_event"), "fixed_event", out.events.size(), false);
    for (const Json& pool : r.array(file, "planet_pools")) {
        if (!pool.is_array()) r.fail("a planet pool is not a list");
        std::vector<std::uint8_t> numbers;
        for (const Json& n : pool.array()) numbers.push_back(static_cast<std::uint8_t>(r.whole(n, "a planet number", 255)));
        out.planet_pools.push_back(std::move(numbers));
    }
    return out;
}

}  // namespace mw1o::import

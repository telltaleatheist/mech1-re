// The fields of MW_MAIN's save block (1090:0448..0B5F, CAMPAIGN_SAVE_BYTES),
// by the save codec's names (MW_MAIN_SAVE_FORMAT.cpp), with their offsets in
// the block. new_campaign.json is keyed by them.
//
// new_campaign_fields.cpp is written by tools/import/make_new_campaign_fields.py.
#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace mw1o::rules {

struct NewCampaignField {
    enum class Kind { Byte, Word, Words, Bytes };
    std::string_view name;
    std::uint16_t offset;
    std::uint16_t length;  // bytes
    Kind kind;
};

std::span<const NewCampaignField> new_campaign_fields();

}  // namespace mw1o::rules

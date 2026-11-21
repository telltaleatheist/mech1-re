// The menu actions data/ui names: the continuation enums' members
// (mechwarrior/campaign_continuations.h), each a native 081B code offset the
// campaign's dispatcher knows. ui_actions.cpp is written by
// tools/import/make_ui_actions.py.
#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace mw1o::ui {

struct UiAction {
    std::string_view name;   // "continue.mech_complex"
    std::uint16_t value;     // CAMPAIGN_CONTINUE_MECH_COMPLEX
};

std::span<const UiAction> ui_actions();

}  // namespace mw1o::ui

// The game's door to the layout: read once at start, then held as the exe's
// bytes for the importers in Original/Memory, which run again on every EXEC.
#include "mechwarrior/ui_layout.h"

#include <cstdio>
#include <exception>

#include "Ui.h"
#include "UiTable.h"

namespace {
mw1o::support::NativeStore g_layout(mw1o::ui::ui_table(), "layout");
}

bool MwHostPrepareUi(const char *dataDirectory)
{
    try {
        g_layout.hold(mw1o::ui::load_layout(dataDirectory));
        return true;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "MechWarrior: %s\n", e.what());
        return false;
    }
}

bool MwUiCopyNative(const char *id, uint8_t *bytes, size_t size) { return g_layout.copy_native(id, bytes, size); }
bool MwUiCopyWords(const char *id, uint16_t *words, size_t count, size_t at) { return g_layout.copy_words(id, words, count, at); }
bool MwUiCopyBytes(const char *id, uint8_t *bytes, size_t count, size_t at) { return g_layout.copy_bytes(id, bytes, count, at); }
size_t MwUiPieceSize(const char *id) { return g_layout.size(id); }

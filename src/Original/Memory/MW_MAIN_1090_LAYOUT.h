/* Installation decoders shared by MW_MAIN's screen importers: captions,
 * panels and menus from the layout (data/ui, src/Ui). Boundary code, not
 * original game helpers. */
#ifndef MW_MAIN_1090_LAYOUT_H
#define MW_MAIN_1090_LAYOUT_H
#include "mechwarrior/campaign_service.h"
#include "mechwarrior/campaign_ui.h"
#include <stddef.h>
#include <stdint.h>

/* A caption: its head (x, y, colour) from the layout piece `id`, and its
 * text through its NUL, or exactly `bytes` bytes where the row is a
 * fixed-width template the code fills in. The text is the word `<id>.text`
 * (story/words.json) when the game's table has one (slice 5 moves the
 * screens one by one: `captions <screen>` in src/Import/word_pieces.txt),
 * and otherwise still the exe's, after the head.
 * `offset` is the piece's place in the exe, filled in by MwImportUiCaptions. */
struct UiCaptionOwner
{
    const char *id;
    MwPanelText *display;
    char *text;
    size_t capacity;
    uint16_t offset;
    size_t bytes;
};
bool MwImportUiCaptions(const uint8_t *data,UiCaptionOwner *owners,size_t count);
/* A panel: its header, and its lines through the native line pointers the
 * layout rebuilds from its caption names. A pointer naming none is refused. */
bool MwImportUiPanel(const char *id,MwPanelDescriptor *panel,uint16_t lineCount,const UiCaptionOwner *owners,size_t count);
/* A list panel: `lineCount` line pointers, while its header counts the lines
 * it starts out showing (no more than that); code changes it as rows fill. */
bool MwImportUiListPanel(const char *id,MwPanelDescriptor *panel,uint16_t lineCount,const UiCaptionOwner *owners,size_t count);
/* A menu's records. A text record's word is the address of its caption's
 * colour byte (the head's last), so it names the caption; zero leaves the
 * text for code to fill in, as do picture records. */
bool MwImportUiMenu(const char *id,MwUiMenu *menu,const UiCaptionOwner *owners,size_t count);
#endif

/* story_span.h: runs of words and native spans, read in bounds. */
#include "mechwarrior/story_span.h"
#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/story.h"
#include "../Import/ImportTable.h"
#include <algorithm>
#include <stdio.h>
#include <string.h>

using mw1o::import::ImportPiece;

[[noreturn]] static void StorySpanFault(const std::string &name,const char *what)
{
    char reason[320];
    snprintf(reason,sizeof(reason),"Story data: \"%s\" %s. This is a fault of the program",name.c_str(),what);
    MW_PORT_FAILSAFE(reason);
}
void MwStorySpan::fault(const char *what) const {StorySpanFault(name_,what);}

static bool Names(std::string_view wanted,std::string_view id)
{
    return wanted.ends_with('.')?id.starts_with(wanted):id==wanted;
}
MwStorySpan MwStorySpan::words(std::initializer_list<const char *> ids)
{
    std::string name;
    std::vector<const ImportPiece *> pieces;
    for (const char *wanted:ids)
    {
        if (!name.empty()) name+=", ";
        name+=wanted;
        const size_t before=pieces.size();
        for (const ImportPiece &piece:mw1o::import::import_table())
            if (mw1o::import::is_word(piece.kind) && Names(wanted,piece.id)) pieces.push_back(&piece);
        if (pieces.size()==before) StorySpanFault(wanted,"names no word");
    }
    std::sort(pieces.begin(),pieces.end(),[](const ImportPiece *a,const ImportPiece *b) {return a->offset<b->offset;});
    const ImportPiece *first=pieces.front();
    size_t length=0;
    for (const ImportPiece *piece:pieces)
    {
        if (piece->exe!=first->exe || piece->segment!=first->segment || piece->offset!=first->offset+length)
            StorySpanFault(name,("has a gap or an overlap before \""+std::string(piece->id)+"\"").c_str());
        length+=piece->length;
    }
    std::vector<uint8_t> bytes(length);
    for (const ImportPiece *piece:pieces)
    {
        const size_t at=piece->offset-first->offset;
        if (!MwWordsCopyBlock(std::string(piece->id).c_str(),bytes.data()+at,piece->length))
            StorySpanFault(name,("has no word \""+std::string(piece->id)+"\"").c_str());
    }
    return MwStorySpan(name,first->offset,std::move(bytes));
}
MwStorySpan MwStorySpan::native(const char *id,size_t size)
{
    std::vector<uint8_t> bytes(size);
    if (!MwStoryCopyBytes(id,bytes.data(),size))
    {
        char what[64];
        snprintf(what,sizeof(what),"has no span of %zu bytes",size);
        StorySpanFault(id,what);
    }
    return MwStorySpan(id,0,std::move(bytes));
}
const uint8_t *MwStorySpan::bytes(size_t at,size_t count) const
{
    if (at>bytes_.size() || count>bytes_.size()-at)
    {
        char what[120];
        snprintf(what,sizeof(what),"is %zu bytes, read at %zu (%zu bytes)",bytes_.size(),at,count);
        fault(what);
    }
    return bytes_.data()+at;
}
void MwStorySpan::copy(void *to,size_t count,size_t at) const {memcpy(to,bytes(at,count),count);}
void MwStorySpan::copyAll(void *to,size_t count) const
{
    if (count!=bytes_.size())
    {
        char what[120];
        snprintf(what,sizeof(what),"is %zu bytes, not %zu",bytes_.size(),count);
        fault(what);
    }
    copy(to,count,0);
}
size_t MwStorySpan::index(uint16_t nearOffset) const
{
    if (!holds(nearOffset))
    {
        char what[64];
        snprintf(what,sizeof(what),"(%04X) does not hold %04X",base_,nearOffset);
        fault(what);
    }
    return nearOffset-base_;
}
bool MwStorySpan::holdsText(uint16_t nearOffset) const
{
    return holds(nearOffset) && memchr(bytes_.data()+(nearOffset-base_),0,bytes_.size()-(nearOffset-base_));
}
const char *MwStorySpan::text(size_t at) const
{
    const uint8_t *from=bytes(at,1);
    if (!memchr(from,0,bytes_.size()-at)) fault("holds a text without its end");
    return (const char *)from;
}
bool MwWordsCopyTextAt(const char *prefix,uint16_t nearOffset,char *text,size_t capacity)
{
    for (const ImportPiece &piece:mw1o::import::import_table())
        if (mw1o::import::is_string(piece.kind) && mw1o::import::is_word(piece.kind) && piece.offset==nearOffset
            && piece.id.starts_with(prefix))
            return MwWordsCopyText(std::string(piece.id).c_str(),text,capacity);
    fprintf(stderr,"MechWarrior: no word \"%s*\" starts at %04X\n",prefix,nearOffset);
    return false;
}

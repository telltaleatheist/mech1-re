#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_heap.h"
#include <stdlib.h>

/* Combat's open files (0810's FILE table; B5). A stream keeps its slot of the table, because the slot's address
 * (BtechStreamIdentity) is game-visible: the sound bank's goes into the checksum that seeds the combat random mix
 * (1ADF:02F8, 0810:2C40). The image's standard streams hold slots 0..4 and a file takes the lowest free one, which
 * runtime_test pins. The DOS handles under them (0810's handle table and limit, the temp-file ids, the external
 * buffers) are gone: host stdio does the IO, and the table's fifteen free slots were the handles' limit too. */
uint16_t BtechStreamIdentity(const BtechRuntimeStream *stream)
{
    return (uint16_t)(COMBAT_STREAM_TABLE+(stream-combatSound.runtimeStreams)*COMBAT_STREAM_STRIDE);
}

/*0810:F948. NULL when every slot is in use (the last is inclusive). */
BtechRuntimeStream *BtechRuntimeStreamFindFreeSlot(void)
{
    for (uint16_t slot=0;slot<COMBAT_STREAM_SLOTS;++slot)
    {
        BtechRuntimeStream *stream=&combatSound.runtimeStreams[slot];
        if (stream->flags&COMBAT_STREAM_ACTIVE_MASK) continue;
        stream->flags=0;stream->buffer=NULL;
        return stream;
    }
    return NULL;
}

/*0810:E2C0. Binary-read resource branch. F7E2's other modes and runtime buffer
 * error flags are not yet promoted; do not disguise them as host fopen behavior. */
BtechRuntimeStream *BtechRuntimeStreamOpen(const char *filename,const char *mode)
{
    BtechRuntimeStream *stream=BtechRuntimeStreamFindFreeSlot();
    if (!stream) return NULL;
    if ((mode[0]!='r' && mode[0]!='w') || mode[1]!='b' || mode[2]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (mode[0]!='r' && mode[0]!='w') || mode[1]!='b' || mode[2]");
    if (!btechHostOpenStream) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostOpenStream");
    stream->file=btechHostOpenStream(filename,mode);
    if (!stream->file) return NULL;
    stream->flags=mode[0]=='r'?COMBAT_STREAM_READ:COMBAT_STREAM_WRITE;
    return stream;
}

/*0810:E206. Host stdio owns resource buffering; preserve active/protected
 * tests, flush-before-close status, and unconditional final flags clear. */
int16_t BtechRuntimeStreamClose(BtechRuntimeStream *stream)
{
    int16_t closeStatus=-1;
    if ((stream->flags&COMBAT_STREAM_ACTIVE_MASK) && !(stream->flags&COMBAT_STREAM_PROTECTED))
    {
        closeStatus=BtechRuntimeStreamFlush(stream);
        BtechRuntimeStreamReleaseBuffer(stream);
        if (!stream->file) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("A standard stream closed by the game.");
        if (fclose(stream->file)) closeStatus=-1;
        stream->file=NULL;
    }
    stream->flags=0;
    return closeStatus;
}

/*F776. A failed512-byte allocation sets the original unbuffered flag. Host
 * stdio performs the actual IO; its opaque buffer is not a second game owner. */
void BtechRuntimeStreamAllocateBuffer(BtechRuntimeStream *stream)
{
    stream->buffer=static_cast<uint8_t *>(BtechAllocateNearBytes(BTECH_NEAR_STREAM_BUFFER_BYTES,BTECH_NEAR_STREAM_BUFFER_BYTES,BTECH_NEAR_STREAM_BUFFER));
    stream->flags|=stream->buffer?COMBAT_STREAM_OWNS_BUFFER:COMBAT_STREAM_UNBUFFERED;
}
/*F748. Stream close uses plain E96E, NOT CDD9's search-reset probe. */
void BtechRuntimeStreamReleaseBuffer(BtechRuntimeStream *stream)
{
    if ((stream->flags&COMBAT_STREAM_ACTIVE_MASK) && (stream->flags&COMBAT_STREAM_OWNS_BUFFER))
    {
        BtechReleaseNearHeapBlock(stream->buffer);stream->buffer=NULL;
        stream->flags&=(uint8_t)~COMBAT_STREAM_OWNS_BUFFER;
    }
}
/*CD16 and getc's E502 refill boundary. Reserve buffering at the first read,
 * not at fopen: this allocation order is observable to the shared near heap. */
size_t BtechReadStreamElements(void *destination,size_t size,size_t count,FILE *file)
{
    if (size && count)
        for (uint16_t slot=0;slot<COMBAT_STREAM_SLOTS;++slot)
            if (combatSound.runtimeStreams[slot].file==file)
            {
                if (!(combatSound.runtimeStreams[slot].flags&(COMBAT_STREAM_OWNS_BUFFER|COMBAT_STREAM_UNBUFFERED)))
                    BtechRuntimeStreamAllocateBuffer(&combatSound.runtimeStreams[slot]);
                break;
            }
    return fread(destination,size,count,file);
}
int BtechReadStreamByte(FILE *file)
{
    uint8_t value;return BtechReadStreamElements(&value,1,1,file)==1?value:EOF;
}
int BtechCloseStreamFile(FILE *file)
{
    for (uint16_t slot=0;slot<COMBAT_STREAM_SLOTS;++slot)
        if (combatSound.runtimeStreams[slot].file==file) return BtechRuntimeStreamClose(&combatSound.runtimeStreams[slot]);
    return fclose(file); /* External FILE supplied by an isolated runtime caller. */
}

/*0810:F8DA. Host stdio collapses the read-buffer cursor/count reset. Binary
 * resource reads and unbuffered authored standard FILEs need no output flush.
 * A buffered writer's remains gated: none is opened. */
int16_t BtechRuntimeStreamFlush(BtechRuntimeStream *stream)
{
    if ((stream->flags&(COMBAT_STREAM_READ|COMBAT_STREAM_WRITE))==COMBAT_STREAM_WRITE && (stream->flags&COMBAT_STREAM_OWNS_BUFFER)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("A buffered writer's flush.");
    return 0;
}

/*0810:FE80. Real authored callback at4DFA, absent from Reko's procedure list.
 * Ascending inclusive FILE scan; count successful FLUSHES. No FILEs close. */
uint16_t BtechFlushAllRuntimeStreams(void)
{
    uint16_t flushed=0;
    for (BtechRuntimeStream &stream:combatSound.runtimeStreams)
        if ((stream.flags&COMBAT_STREAM_ACTIVE_MASK) && BtechRuntimeStreamFlush(&stream)!=-1) ++flushed;
    return flushed;
}

/* Every stream's host file closed (0810:E144's raw handle close, and the child's disposal). The flags stay, as the
 * raw close left them. */
void BtechCloseRuntimeStreamFiles(void)
{
    for (BtechRuntimeStream &stream:combatSound.runtimeStreams)
        if (stream.file) {fclose(stream.file);stream.file=NULL;}
}

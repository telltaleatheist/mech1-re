#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_resources.h"
#include "mechwarrior/combat_heap.h"
#include <stdlib.h>
#include <string.h>

/* File-read methods00/01/02. Memory archives and archive writers are not
 * reachable from the scenario bank importer and remain explicitly unsupported.
 * No declared-length cap: native44DA decodes until compressed EOF. */

/*1BF5:3D49. Failed/null selection leaves other globals stale. Native unchecked
 * table indexing beyond its100 slots is unrelated-owner corruption. */
uint16_t BtechSelectArchiveDescriptor(uint16_t handle)
{
    if (handle>=RESOURCE_ARCHIVE_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: handle>=RESOURCE_ARCHIVE_SLOTS");
    combatResources.selectedArchive=combatResources.archiveDescriptors[handle];
    if (!combatResources.selectedArchive) return 0;
    if (!(combatResources.selectedArchive->methodAndFlags&RESOURCE_FILE_FLAG)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !(selectedArchive->methodAndFlags&RESOURCE_FILE_FLAG)");
    combatResources.archiveIoFlags=RESOURCE_FILE_FLAG;
    return 1;
}

/*1BF5:3DEF. Lowercase r anywhere, not a host fopen mode validator. */
uint16_t BtechArchiveModeContainsRead(const char *mode)
{
    while (*mode) if (*mode++=='r') return 1;
    return 0;
}

/*1BF5:3E26. Ordinary runtime allocation release is the low-level boundary. */
void BtechReleaseNearArchiveBlockIfNonzero(void *allocation)
{
    if (allocation) BtechReleaseNearBlockAndProbeAllocation(allocation);
}

/*1BF5:3E49. State then owned DOS workspace then descriptor; borrowed FILE
 * stays open. Selected native reference is stale after release, not cleared. */
uint16_t BtechReleaseArchiveDescriptor(uint16_t handle)
{
    if (handle>=RESOURCE_ARCHIVE_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: handle>=RESOURCE_ARCHIVE_SLOTS");
    combatResources.selectedArchive=combatResources.archiveDescriptors[handle];
    if (combatResources.selectedArchive)
    {
        BtechReleaseNearArchiveBlockIfNonzero(combatResources.selectedArchive->state);
        if (combatResources.selectedArchive->workspace && !combatResources.borrowedArchiveWorkspace)
        {
            BtechFreeDosMemorySelector((uint8_t *)combatResources.selectedArchive->workspace);
        }
    }
    BtechReleaseNearArchiveBlockIfNonzero(combatResources.selectedArchive);
    combatResources.archiveDescriptors[handle]=NULL;
    return UINT16_MAX;
}

/*1BF5:3EC0. Full table leaves selection stale; failed calloc sets it NULL. */
int16_t BtechAllocateArchiveDescriptor(const char *ignoredMode)
{
    (void)ignoredMode;
    uint16_t handle=0;
    while (handle<RESOURCE_ARCHIVE_SLOTS && combatResources.archiveDescriptors[handle]) ++handle;
    if (handle==RESOURCE_ARCHIVE_SLOTS) return -1;
    combatResources.selectedArchive=static_cast<BtechArchiveDescriptor *>(BtechAllocateZeroedNearElements(1,BTECH_NEAR_ARCHIVE_BYTES,sizeof(*combatResources.selectedArchive),BTECH_NEAR_ARCHIVE));
    if (!combatResources.selectedArchive) return -1;
    combatResources.archiveDescriptors[handle]=combatResources.selectedArchive;
    return (int16_t)handle;
}

/*1BF5:3F3A read-method02 branch. State is near calloc; the separate workspace
 * really passes through DOS paragraph allocation, not a replacement heap. */
uint16_t BtechAllocateArchiveMethodState(int16_t method,const char *mode)
{
    if (method>RESOURCE_METHOD_LZW) return UINT16_MAX;
    if (method<RESOURCE_METHOD_RAW || !BtechArchiveModeContainsRead(mode)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: method<RESOURCE_METHOD_RAW || !BtechArchiveModeContainsRead(mode)");
    combatResources.selectedArchive->state=static_cast<uint8_t *>(BtechAllocateZeroedNearElements(BTECH_NEAR_ARCHIVE_STATE_BYTES,1,BTECH_NEAR_ARCHIVE_STATE_BYTES,BTECH_NEAR_BYTES));
    if (!combatResources.selectedArchive->state) return UINT16_MAX;
    if (method==RESOURCE_METHOD_LZW)
    {
        combatResources.selectedArchive->workspace=combatResources.borrowedArchiveWorkspace;
        if (!combatResources.selectedArchive->workspace)
            combatResources.selectedArchive->workspace=(BtechLzwReaderWorkspace *)BtechAllocateDosMemoryBlock(0x3AB3).bytes;
        if (!combatResources.selectedArchive->workspace) return UINT16_MAX;
        if (combatResources.selectedArchive->workspace!=combatResources.borrowedArchiveWorkspace)
        {
            if (!btechHostDosAllocationSelector) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostDosAllocationSelector");
            combatResources.selectedArchive->workspaceSelector=btechHostDosAllocationSelector((const uint8_t *)combatResources.selectedArchive->workspace);
        }
    }
    combatResources.selectedArchive->methodAndFlags=(uint8_t)method;
    return (uint16_t)method; /* Only FFFF/non-FFFF is consumed; native incidental AH is not host ABI. */
}

/*1BF5:4031. Descriptor calloc supplies untouched partial header bytes. */
int16_t BtechOpenCompressedArchive(uint16_t method,FILE *stream,const char *mode,uint32_t physicalBytes)
{
    int16_t handle=BtechAllocateArchiveDescriptor(mode);
    if (handle==-1) return -1;
    BtechArchiveDescriptor *archive=combatResources.selectedArchive;
    archive->stream=stream;
    for (uint16_t slot=0;slot<COMBAT_STREAM_SLOTS;++slot)
        if (combatSound.runtimeStreams[slot].file==stream)
        {archive->streamIdentity=BtechStreamIdentity(&combatSound.runtimeStreams[slot]);break;}
    archive->initialStreamPosition=(uint32_t)ftell(stream);
    archive->compressedPosition=5;
    if (!BtechArchiveModeContainsRead(mode)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !BtechArchiveModeContainsRead(mode)"); /* Unconverted writer. */
    method=(uint8_t)BtechReadStreamByte(stream);
    archive->methodAndFlags=(uint8_t)method;
    if (BtechAllocateArchiveMethodState((int16_t)method,mode)==UINT16_MAX)
    {
        fseek(stream,-1,SEEK_CUR);
        return (int16_t)BtechReleaseArchiveDescriptor((uint16_t)handle);
    }
    archive->compressedBytes=physicalBytes;
    uint8_t lengthBytes[4]={0};
    BtechReadStreamElements(lengthBytes,1,4,stream);
    archive->decodedBytes=(uint32_t)lengthBytes[0]|((uint32_t)lengthBytes[1]<<8)
        |((uint32_t)lengthBytes[2]<<16)|((uint32_t)lengthBytes[3]<<24);
    if (method==RESOURCE_METHOD_LZW) BtechInitializeLzwReader();
    archive->methodAndFlags|=RESOURCE_READ_FLAG|RESOURCE_FILE_FLAG;
    return handle;
}

/*1BF5:42F7 read branch. The borrowed FILE is NOT closed here. */
uint16_t BtechFlushAndCloseCompressedArchive(uint16_t handle)
{
    if (!BtechSelectArchiveDescriptor(handle)) return UINT16_MAX;
    if (!(combatResources.selectedArchive->methodAndFlags&RESOURCE_READ_FLAG)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !(selectedArchive->methodAndFlags&RESOURCE_READ_FLAG)");
    BtechReleaseArchiveDescriptor(handle);
    return 0;
}

/*1BF5:45FA. No decoding or seek occurs. */
uint32_t BtechGetArchiveDecodedLength(uint16_t handle)
{
    if (!BtechSelectArchiveDescriptor(handle)) return UINT32_MAX;
    return combatResources.selectedArchive->decodedBytes;
}

/*1BF5:3A97 file branch. Requested/clamped count is native WORD.
 * fread writes only the returned prefix; packet tail deliberately stays stale. */
uint16_t BtechReadArchiveSourceBytes(uint8_t *destination,uint16_t bytes)
{
    BtechArchiveDescriptor *archive=combatResources.selectedArchive;
    uint32_t remaining=archive->compressedBytes-archive->compressedPosition;
    if (remaining<bytes) bytes=(uint16_t)remaining;
    if (!remaining) return 0;
    archive->compressedPosition+=bytes; /* BEFORE the possibly short read. */
    return BtechReadStreamElements(destination,1,bytes,archive->stream);
}

/*1BF5:3C82. WORD increment of the two BYTE cursors retains the carry alias. */
uint16_t BtechEmitArchiveByte(uint16_t value)
{
    if (combatResources.archiveRemainingBytes)
    {
        if (combatResources.archiveIoFlags&RESOURCE_READ_FLAG) *combatResources.archiveDestination=(uint8_t)value;
        ++combatResources.archiveDestination;--combatResources.archiveRemainingBytes;return 1;
    }
    BtechArchiveDescriptor *archive=combatResources.selectedArchive;
    uint8_t pendingEnd=archive->pendingEnd;
    uint16_t pendingPair=pendingEnd|((uint16_t)archive->pendingRead<<8);
    ++pendingPair;
    archive->pendingEnd=(uint8_t)pendingPair;archive->pendingRead=(uint8_t)(pendingPair>>8);
    if (pendingEnd>=0x80) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: pendingEnd>=0x80"); /* Native state-allocation overrun is not host memory. */
    archive->state[pendingEnd]=(uint8_t)value;
    return 0;
}

/*1BF5:43E4. Empty pending buffer is reset BEFORE returning. */
void BtechDeliverPendingArchiveBytes(void)
{
    BtechArchiveDescriptor *archive=combatResources.selectedArchive;
    uint16_t pendingRead=archive->pendingRead;
    uint16_t delivered=archive->pendingEnd-pendingRead;
    if (combatResources.archiveRemainingBytes<delivered)
    {delivered=combatResources.archiveRemainingBytes;archive->pendingRead+=(uint8_t)delivered;}
    else {archive->pendingEnd=0;archive->pendingRead=0;}
    if (!delivered) return;
    if (pendingRead+delivered>0x80) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: pendingRead+delivered>0x80");
    if (combatResources.archiveIoFlags&RESOURCE_READ_FLAG) memcpy(combatResources.archiveDestination,archive->state+pendingRead,delivered);
    combatResources.archiveRemainingBytes-=delivered;combatResources.archiveDestination+=delivered;
}

/*1BF5:447B. Exactly one decode callback, with pending delivery on both sides. */
uint16_t BtechDecodeArchiveBytes(uint16_t handle,uint16_t requestedBytes)
{
    (void)handle; /* Genuine unused source argument. */
    combatResources.archiveRemainingBytes=requestedBytes;
    BtechDeliverPendingArchiveBytes();
    if (combatResources.archiveRemainingBytes)
    {
        switch (combatResources.selectedArchive->methodAndFlags&0x1F)
        {
        case RESOURCE_METHOD_RAW:BtechDecodeRawArchive();break;
        case RESOURCE_METHOD_RLE:BtechDecodeRleArchive();break;
        case RESOURCE_METHOD_LZW:BtechDecodeLzwPacketStream();break;
        default:/* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Fatal path under condition: archiveRemainingBytes");
        }
        if (combatResources.archiveRemainingBytes) BtechDeliverPendingArchiveBytes();
    }
    requestedBytes-=combatResources.archiveRemainingBytes;
    combatResources.selectedArchive->decodedPosition+=requestedBytes;
    return requestedBytes;
}

/*1BF5:44DA. Normalizing selector:offset removes hardware address transport.
 * Real caller DC86 uses7D00-byte chunks, never a low-offset wrap in48E6. */
uint16_t BtechReadArchiveIntoFarBuffer(uint16_t handle,uint8_t *destination,uint16_t bytes)
{
    if (!BtechSelectArchiveDescriptor(handle)) return UINT16_MAX;
    if (bytes>0xFFF0) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: bytes>0xFFF0"); /* Outside the proved normalized non-wrapping request domain. */
    combatResources.archiveDestination=destination;
    combatResources.archiveIoFlags|=RESOURCE_READ_FLAG;
    return BtechDecodeArchiveBytes(handle,bytes);
}

/*1BF5:3A23. Exact compressed-position equality, not a >= length cap. The
 * position advances before getc even when the underlying file is already EOF. */
uint16_t BtechReadArchiveSourceByte(void)
{
    BtechArchiveDescriptor *archive=combatResources.selectedArchive;
    if (archive->compressedPosition==archive->compressedBytes) return UINT16_MAX;
    ++archive->compressedPosition;
    return (uint16_t)BtechReadStreamByte(archive->stream);
}

/*1BF5:38BC. Read and buffer one extra byte after satisfying the request. */
void BtechDecodeRawArchive(void)
{
    uint16_t keepReading=1;
    while (keepReading)
    {
        uint16_t value=BtechReadArchiveSourceByte();
        if (value==UINT16_MAX) break;
        keepReading=BtechEmitArchiveByte(value);
    }
}

/*1BF5:38EF. Method01 FILE path. EOF repeat value is deliberately not rejected;
 * buffering an entire run lets447B deliver only the caller's requested prefix. */
void BtechDecodeRleArchive(void)
{
    if (!(combatResources.archiveIoFlags&RESOURCE_FILE_FLAG)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !(archiveIoFlags&RESOURCE_FILE_FLAG)");
    uint16_t keepReading=1;
    while (keepReading)
    {
        uint16_t control=BtechReadArchiveSourceByte();
        if (control==UINT16_MAX) break;
        if (control&0x80)
        {
            uint16_t count=control&0x7F;
            uint16_t value=BtechReadArchiveSourceByte();
            keepReading=BtechDecodeRleRepeatedRun(value,count);
        }
        else keepReading=BtechDecodeRleLiteralRun(control&0x7F);
    }
}

/*1BF5:39A2. Native50-byte IO staging; short reads are not retried. */
void BtechCopyArchiveStreamBytes(uint8_t *destination,uint16_t bytes)
{
    int16_t read=1;
    while (bytes && read>0)
    {
        uint16_t request=bytes<sizeof(combatResources.archiveStreamScratch)?bytes:(uint16_t)sizeof(combatResources.archiveStreamScratch);
        read=(int16_t)BtechReadStreamElements(combatResources.archiveStreamScratch,1,request,combatResources.selectedArchive->stream);
        bytes-=(uint16_t)read;
        memcpy(destination,combatResources.archiveStreamScratch,(uint16_t)read);
        destination+=(uint16_t)read;
    }
}

/*1BF5:3B5A. Advance compressed position before IO; requested count, not read
 * count, advances the caller's output. Buffered literals overwrite state BASE. */
uint16_t BtechDecodeRleLiteralRun(uint16_t bytes)
{
    BtechArchiveDescriptor *archive=combatResources.selectedArchive;
    archive->compressedPosition+=bytes;
    if (combatResources.archiveRemainingBytes>=bytes)
    {
        if (combatResources.archiveIoFlags&RESOURCE_READ_FLAG) BtechCopyArchiveStreamBytes(combatResources.archiveDestination,bytes);
        else fseek(archive->stream,bytes,SEEK_CUR);
        combatResources.archiveRemainingBytes-=bytes;combatResources.archiveDestination+=bytes;
        return 1;
    }
    archive->pendingEnd+=(uint8_t)bytes;
    if (bytes>0x80) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: bytes>0x80");
    BtechCopyArchiveStreamBytes(archive->state,bytes);
    return 0;
}

/*1BF5:3BEE. Whole repeated runs are buffered when the request is smaller.
 * Direct signed-high count/CWD overrun callers remain outside this decoder. */
uint16_t BtechDecodeRleRepeatedRun(uint16_t value,uint16_t bytes)
{
    if (bytes<=combatResources.archiveRemainingBytes)
    {
        if (combatResources.archiveIoFlags&RESOURCE_READ_FLAG) memset(combatResources.archiveDestination,(uint8_t)value,bytes);
        combatResources.archiveRemainingBytes-=bytes;combatResources.archiveDestination+=bytes;
        return 1;
    }
    BtechArchiveDescriptor *archive=combatResources.selectedArchive;
    if ((uint16_t)archive->pendingEnd+bytes>0x80) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)archive->pendingEnd+bytes>0x80");
    memset(archive->state+archive->pendingEnd,(uint8_t)value,bytes);
    archive->pendingEnd+=(uint8_t)bytes;
    return 0;
}

/*1BF5:5334. Only3AA1 bytes clear; the final12 bytes of the reverse workspace
 * remain allocation residue. Native DOS workspace has offset0000 and DF=0. */
void BtechInitializeLzwReader(void)
{
    static_assert(offsetof(BtechLzwReaderWorkspace,suffixBytes)==0x2720,"Native suffix workspace");
    static_assert(offsetof(BtechLzwReaderWorkspace,reversePhraseBytes)==0x3720,"Native reverse workspace");
    BtechLzwReaderWorkspace *workspace=combatResources.selectedArchive->workspace;
    memset(workspace,0,0x3AA1);
    for (uint16_t literal=0;literal<0x100;++literal) workspace->suffixBytes[literal]=(uint8_t)literal;
    combatResources.lzwCodeWidth=LZW_INITIAL_CODE_BITS;combatResources.lzwCodeWidthLimit=0x01FF;combatResources.lzwNextFreeCode=LZW_FIRST_CODE;
    combatResources.lzwClearPending=0;combatResources.lzwInitialLiteralPending=1;combatResources.lzwPhraseResumePending=0;
    combatResources.lzwPacketBitOffset=0;combatResources.lzwPacketBitLimit=0;
}

/*1BF5:4AE9. Width-byte packets, not a continuously packed bitstream. This
 * expression preserves the native AH residue while extracting the first byte. */
uint16_t BtechReadLzwCode(void)
{
    if ((int16_t)combatResources.lzwNextFreeCode>(int16_t)combatResources.lzwCodeWidthLimit)
    {
        ++combatResources.lzwCodeWidth;
        combatResources.lzwCodeWidthLimit=combatResources.lzwCodeWidth==LZW_MAX_CODE_BITS?LZW_DICTIONARY_LIMIT:(uint16_t)((1u<<combatResources.lzwCodeWidth)-1);
        goto refillPacket;
    }
    if (combatResources.lzwClearPending) goto refillPacket;
    if ((int16_t)combatResources.lzwPacketBitOffset>=(int16_t)combatResources.lzwPacketBitLimit) goto readPacket;
    goto extractCode;
refillPacket:
    if (combatResources.lzwClearPending)
    {combatResources.lzwCodeWidth=LZW_INITIAL_CODE_BITS;combatResources.lzwCodeWidthLimit=0x01FF;combatResources.lzwClearPending=0;}
readPacket:;
    if (combatResources.lzwCodeWidth>sizeof(combatResources.lzwPacketBytes)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: lzwCodeWidth>sizeof(lzwPacketBytes)");
    uint16_t readBytes;readBytes=BtechReadArchiveSourceBytes(combatResources.lzwPacketBytes,combatResources.lzwCodeWidth);
    if (!readBytes || (readBytes&0x8000)) {combatResources.lzwPacketBitLimit=readBytes;return UINT16_MAX;}
    combatResources.lzwPacketBitOffset=0;
    combatResources.lzwPacketBitLimit=(uint16_t)((readBytes<<3)-(combatResources.lzwCodeWidth-1));
extractCode:;
    uint16_t bitOffset=combatResources.lzwPacketBitOffset,remainingBits=combatResources.lzwCodeWidth;
    combatResources.lzwPacketBitOffset+=remainingBits;
    uint16_t source=bitOffset>>3;
    uint8_t shift=(uint8_t)bitOffset&7;
    if (source>=sizeof(combatResources.lzwPacketBytes)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: source>=sizeof(lzwPacketBytes)");
    uint16_t code=(combatResources.lzwPacketBitOffset&0xFF00)|combatResources.lzwPacketBytes[source++];
    code>>=shift;
    uint16_t combined=code;
    shift=(uint8_t)(8-shift);remainingBits-=shift;
    if (remainingBits>=8)
    {
        if (source>=sizeof(combatResources.lzwPacketBytes)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: source>=sizeof(lzwPacketBytes)");
        code=(uint16_t)((code&0xFF00)|combatResources.lzwPacketBytes[source++]);
        combined|=(uint16_t)(code<<shift);shift+=8;remainingBits-=8;
    }
    if (!remainingBits) return combined; /* Native final mask00 reads no contributing bits. */
    if (source>=sizeof(combatResources.lzwPacketBytes)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: source>=sizeof(lzwPacketBytes)");
    return (uint16_t)(combined|((combatResources.lzwPacketBytes[source]&((1u<<remainingBits)-1))<<shift));
}

/*1BF5:48E6. Native reverse phrase, one-byte overread/pending and deferred
 * dictionary insertion are retained. It is not the bounded research decoder. */
void BtechDecodeLzwPacketStream(void)
{
    BtechLzwReaderWorkspace *workspace=combatResources.selectedArchive->workspace;
    uint16_t reverseRead,reverseCount,code,prefixCode;
    if (combatResources.lzwPhraseResumePending)
    {
        reverseRead=combatResources.lzwReverseReadOffset;combatResources.lzwPhraseResumePending=0;
        goto nextReverseByte;
    }
    if (combatResources.lzwInitialLiteralPending)
    {
        combatResources.lzwInitialLiteralPending=0;
        combatResources.lzwPreviousCode=BtechReadLzwCode();combatResources.lzwPhraseFirstByte=combatResources.lzwPreviousCode;
        BtechEmitArchiveByte(combatResources.lzwPreviousCode); /* FFFF is unchecked here. */
    }
readNextCode:
    code=BtechReadLzwCode();
    if (code&0x8000) return;
    if (code==LZW_CLEAR_CODE)
    {
        memset(workspace->prefixCodes,0,0x200);
        combatResources.lzwClearPending=1;combatResources.lzwNextFreeCode=LZW_CLEAR_CODE;
        code=BtechReadLzwCode();if (code&0x8000) return;
    }
    reverseCount=0;prefixCode=code;combatResources.lzwIncomingCode=code;
    if ((int16_t)code>=(int16_t)combatResources.lzwNextFreeCode)
    {workspace->reversePhraseBytes[reverseCount++]=(uint8_t)combatResources.lzwPhraseFirstByte;prefixCode=combatResources.lzwPreviousCode;}
    while ((int16_t)prefixCode>=LZW_CLEAR_CODE)
    {
        if (prefixCode>=LZW_DICTIONARY_LIMIT || reverseCount>=sizeof(workspace->reversePhraseBytes)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: prefixCode>=LZW_DICTIONARY_LIMIT || reverseCount>=sizeof(workspace->reversePhraseBytes)");
        workspace->reversePhraseBytes[reverseCount++]=workspace->suffixBytes[prefixCode];
        prefixCode=workspace->prefixCodes[prefixCode];
    }
    if (prefixCode>=LZW_DICTIONARY_LIMIT || reverseCount>=sizeof(workspace->reversePhraseBytes)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: prefixCode>=LZW_DICTIONARY_LIMIT || reverseCount>=sizeof(workspace->reversePhraseBytes)");
    combatResources.lzwPhraseFirstByte=workspace->suffixBytes[prefixCode];
    workspace->reversePhraseBytes[reverseCount++]=(uint8_t)combatResources.lzwPhraseFirstByte;
    reverseRead=reverseCount-1;
emitReverseByte:;
    uint8_t phraseByte;phraseByte=workspace->reversePhraseBytes[reverseRead++];
    if (!combatResources.archiveRemainingBytes)
    {
        combatResources.lzwReverseReadOffset=reverseRead;BtechEmitArchiveByte(phraseByte);
        combatResources.lzwPhraseResumePending=1;return;
    }
    if (combatResources.archiveIoFlags&RESOURCE_READ_FLAG) *combatResources.archiveDestination=phraseByte;
    ++combatResources.archiveDestination;--combatResources.archiveRemainingBytes;
nextReverseByte:
    reverseRead-=2;
    if (!(reverseRead&0x8000)) goto emitReverseByte;
    if ((int16_t)combatResources.lzwNextFreeCode<LZW_DICTIONARY_LIMIT)
    {
        if (combatResources.lzwNextFreeCode>=LZW_DICTIONARY_LIMIT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: lzwNextFreeCode>=LZW_DICTIONARY_LIMIT");
        workspace->prefixCodes[combatResources.lzwNextFreeCode]=combatResources.lzwPreviousCode;
        workspace->suffixBytes[combatResources.lzwNextFreeCode++]=(uint8_t)combatResources.lzwPhraseFirstByte;
    }
    combatResources.lzwPreviousCode=combatResources.lzwIncomingCode;
    goto readNextCode;
}

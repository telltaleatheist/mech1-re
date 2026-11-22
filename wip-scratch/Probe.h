// SCRATCH PROBE (slice 6 planning): dumps the __DATA segment to
// $MW1O_PROBE_DIR/<n>-<tag>.bin; never committed.
#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>
inline void ProbeDump(const char *tag)
{
    static int sequence=0;
    const char *dir=getenv("MW1O_PROBE_DIR");
    if (!dir) return;
    unsigned long size=0;
    uint8_t *at=getsegmentdata(&_mh_execute_header,"__DATA",&size);
    char path[4096];snprintf(path,sizeof(path),"%s/%02d-%s.bin",dir,sequence++,tag);
    FILE *f=fopen(path,"wb");if (!f) return;
    fwrite(at,1,size,f);fclose(f);
}

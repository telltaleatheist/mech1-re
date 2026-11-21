#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/campaign_ui.h"
#include "mechwarrior/campaign_input.h"
#include <stdio.h>

/* MW_MAIN081B:1280..129B (change disk) and129C..12B7 (insert disk one): the
 * original asked for a floppy and retried. A game file that cannot be read is
 * a fatal error naming it; there are no disks to swap. */
void MwFailGameFileRead(const char *filename)
{
    char reason[96];
    snprintf(reason,sizeof(reason),"cannot read the game file %s",filename?filename:"(unknown)");
    MwAbortWithReason("game file",reason,__FILE__,__LINE__,__func__);
}

#include "mechwarrior/video_mode.h"
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/port_diagnostics.h"
#include "Canvas.h"

void MwSetVideoMode(uint8_t mode)
{
    mw1o::video::Canvas &canvas=mw1o::video::GameCanvas();
    if (mode==MW_VIDEO_MODE_320X200)
    {
        canvas.setUp();
        /* The mode set leaves the adapter drawing by copy, and the campaign
         * overlay's sprites draw by the function its last routine left. */
        egaLogicalFunction=EGA_COPY;
    }
    else if (mode==MW_VIDEO_MODE_TEXT) canvas.close();
    else /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Unsupported video mode");
}
uint8_t MwGetVideoMode(void)
{
    return mw1o::video::GameCanvas().shown()?MW_VIDEO_MODE_320X200:MW_VIDEO_MODE_TEXT;
}

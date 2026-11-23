#include "BTECH_2A36_CANVAS.h"
#include "mechwarrior/port_diagnostics.h"
#include <stdlib.h>

int BtechEgaCanvasPage(uint16_t selector)
{
    const uint16_t pageParagraphs=(uint16_t)(mw1o::video::Canvas::PAGE_STRIDE/8/0x10);
    if (selector<COMBAT_EGA_FRONT_PAGE || (selector-COMBAT_EGA_FRONT_PAGE)%pageParagraphs
        || (selector-COMBAT_EGA_FRONT_PAGE)/pageParagraphs>=mw1o::video::Canvas::PAGES)
        /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: a page selector off the canvas's pages");
    return (selector-COMBAT_EGA_FRONT_PAGE)/pageParagraphs;
}

void BtechEgaCanvasPosition(uint16_t byteOffset, int *x, int *y)
{
    const uint32_t pixel=(uint32_t)byteOffset*8;
    *x=(int)(pixel%mw1o::video::Canvas::WIDTH);
    *y=(int)(pixel/mw1o::video::Canvas::WIDTH);
}

void BtechEgaCanvasWriter::setMask(uint8_t mask)
{
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)mask<<8));
    mask_=mask;maskSet_=true;
}

void BtechEgaCanvasWriter::resolve()
{
    if (resolved_) return;
    page_=BtechEgaCanvasPage(selector_);
    logic_=BtechEgaCanvasInheritedLogic();
    BtechEgaWriteState state;
    btechHostReadEgaWriteState(&state);
    if (!maskSet_) mask_=state.bitMask;
    for (int i=0;i<8;++i) latched_[i]=state.latchPixels[i];
    resolved_=true;
}

void BtechEgaCanvasWriter::load(uint16_t cursor)
{
    resolve();
    (void)btechHostReadVideoByte(selector_,cursor);
    int x,y;BtechEgaCanvasPosition(cursor,&x,&y);
    for (int i=0;i<8;++i) latched_[i]=mw1o::video::GameCanvas().pixelAt(page_,x+i,y);
}

void BtechEgaCanvasWriter::write(uint16_t cursor, uint8_t colour)
{
    resolve();
    int x,y;BtechEgaCanvasPosition(cursor,&x,&y);
    mw1o::video::Canvas &canvas=mw1o::video::GameCanvas();
    for (int i=0;i<8;++i)
    {
        canvas.pixel(page_,x+i,y,latched_[i]);
        if (mask_&(0x80>>i)) canvas.pixel(page_,x+i,y,colour,logic_);
    }
}

mw1o::video::Logic BtechEgaCanvasInheritedLogic(void)
{
    if (!btechHostReadEgaWriteState) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostReadEgaWriteState");
    BtechEgaWriteState state;
    btechHostReadEgaWriteState(&state);
    if (state.writeMode!=2 || state.mapMask!=COMBAT_EGA_PLANE_MASK_ALL)
        /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: a canvas path inherited a write mode other than 2 or masked planes");
    static const mw1o::video::Logic logic[4]={mw1o::video::Logic::Copy,mw1o::video::Logic::And,
        mw1o::video::Logic::Or,mw1o::video::Logic::Xor};
    return logic[state.logicalFunction&3];
}

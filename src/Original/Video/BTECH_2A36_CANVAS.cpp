#include "BTECH_2A36_CANVAS.h"
#include "mechwarrior/port_diagnostics.h"
#include <stdlib.h>

using mw1o::video::Canvas;
using mw1o::video::GameCanvas;

int BtechEgaCanvasPage(uint16_t selector)
{
    const uint16_t pageParagraphs=(uint16_t)(Canvas::PAGE_STRIDE/8/0x10);
    if (selector<COMBAT_EGA_FRONT_PAGE || (selector-COMBAT_EGA_FRONT_PAGE)%pageParagraphs
        || (selector-COMBAT_EGA_FRONT_PAGE)/pageParagraphs>=Canvas::PAGES)
        /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: a page selector off the canvas's pages");
    return (selector-COMBAT_EGA_FRONT_PAGE)/pageParagraphs;
}

void BtechEgaCanvasPosition(uint16_t byteOffset, int *x, int *y)
{
    const uint32_t pixel=(uint32_t)byteOffset*8;
    *x=(int)(pixel%Canvas::WIDTH);
    *y=(int)(pixel/Canvas::WIDTH);
}

void BtechEgaCanvasDraw(int page, uint16_t byteOffset, uint8_t mask, uint8_t colour)
{
    int x,y;BtechEgaCanvasPosition(byteOffset,&x,&y);
    GameCanvas().bitPattern(page,x,y,mask,colour);
}

void BtechEgaCanvasSetPixels(int page, uint16_t byteOffset, const uint8_t colours[8])
{
    int x,y;BtechEgaCanvasPosition(byteOffset,&x,&y);
    for (int i=0;i<8;++i) GameCanvas().pixel(page,x+i,y,colours[i]);
}

void BtechEgaCanvasCopy(int fromPage, int toPage, uint16_t byteOffset)
{
    int x,y;BtechEgaCanvasPosition(byteOffset,&x,&y);
    Canvas &canvas=GameCanvas();
    for (int i=0;i<8;++i) canvas.pixel(toPage,x+i,y,canvas.pixelAt(fromPage,x+i,y));
}

uint8_t BtechEgaCanvasColourBits(int page, uint16_t byteOffset, int bit)
{
    int x,y;BtechEgaCanvasPosition(byteOffset,&x,&y);
    uint8_t bits=0;
    for (int i=0;i<8;++i) bits|=(uint8_t)(((GameCanvas().pixelAt(page,x+i,y)>>bit)&1)<<(7-i));
    return bits;
}

void BtechEgaCanvasSetColourBits(int page, uint16_t byteOffset, int bit, uint8_t bits)
{
    int x,y;BtechEgaCanvasPosition(byteOffset,&x,&y);
    Canvas &canvas=GameCanvas();
    for (int i=0;i<8;++i)
    {
        const uint8_t old=canvas.pixelAt(page,x+i,y)&(uint8_t)~(1<<bit);
        canvas.pixel(page,x+i,y,(uint8_t)(old|(((bits>>(7-i))&1)<<bit)));
    }
}

void BtechEgaCanvasAddColourBits(int page, uint16_t byteOffset, int bit, uint8_t bits)
{
    int x,y;BtechEgaCanvasPosition(byteOffset,&x,&y);
    GameCanvas().bitPattern(page,x,y,bits,(uint8_t)(1<<bit),mw1o::video::Logic::Or);
}

void BtechEgaCanvasWriter::write(uint16_t byteOffset, uint8_t colour)
{
    if (!maskSet_) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: a combat write before its mask");
    if (!resolved_) {page_=BtechEgaCanvasPage(selector_);resolved_=true;}
    BtechEgaCanvasDraw(page_,byteOffset,mask_,colour);
}

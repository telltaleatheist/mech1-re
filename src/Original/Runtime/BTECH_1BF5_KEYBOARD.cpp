#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_video.h"
#include <stdlib.h>

/* The key queue: `head` is the next key to read, `tail` the next slot to
 * write, as in the PC BIOS's ring the original queued into (16 slots). */
static uint8_t NextKeySlot(uint8_t slot) { return (uint8_t)((slot+1)%COMBAT_KEY_QUEUE_SLOTS); }

/*1BF5:2CCB, the original's IRQ1 handler, as the handler of one key event (H2).
 * The event is the make/break byte the original read from port 60h; there is
 * no port acknowledge or interrupt controller. The translation, held-key
 * state, shift flags and key queue are the game's own logic and tables. */
void BtechHandleKeyEvent(MwKeyEvent event)
{
    uint8_t scan=MwKeyEventScanByte(event);
    if (pcjrTandyCompatible==1)
    {
        if (keyboardBiosIntercept==1)
        {
            if (scan==COMBAT_KEY_GRAVE) scan=COMBAT_KEY_UP;
            if (scan==COMBAT_KEY_BACKSLASH) scan=COMBAT_KEY_LEFT;
        }
        else
            for (uint16_t index=0;index<COMBAT_KEYBOARD_COMPATIBILITY_CODES;++index)
                if (scan==keyboardCompatibilityScans[index]) {scan=keyboardCompatibilityReplacements[index];break;}
    }
    uint8_t scanIndex=scan&KEYBOARD_SCAN_MASK;
    if (scanIndex>=COMBAT_KEYBOARD_SCAN_CODES) return;
    uint8_t attributes;attributes=keyboardScanTranslations[scanIndex];
    keyboardScanTranslations[scanIndex]=(attributes
        &((scan&KEYBOARD_BREAK_FLAG)?UINT8_MAX:KEYBOARD_ATTRIBUTE_CLEAR_HELD))^KEYBOARD_ATTRIBUTE_HELD;
    if (keyboardBiosIntercept==1)
    {
        if (scanIndex==COMBAT_KEY_CAPS_LOCK) {scan=scanIndex;attributes&=keyboardScanMask;}
        else if (scanIndex==COMBAT_KEY_NUM_LOCK) {scan=scanIndex;attributes&=keyboardExtendedScanMask;}
    }
    if (scan&KEYBOARD_BREAK_FLAG)
    {
        if (attributes&KEYBOARD_ATTRIBUTE_DIRECTION_MASK)
        {
            uint16_t group=(attributes>>KEYBOARD_ATTRIBUTE_GROUP_SHIFT)&KEYBOARD_ATTRIBUTE_GROUP_MASK;
            uint8_t direction=attributes>>KEYBOARD_ATTRIBUTE_DIRECTION_SHIFT;
            if (heldDirectionKeyMasks[group]==direction) heldDirectionKeyMasks[group]=0;
        }
        keyboardLastKey=0;
        uint8_t character=keyboardUnshiftedCharacters[scan&KEYBOARD_SCAN_MASK];
        if ((character&KEYBOARD_CHARACTER_SPECIAL) && !(character&KEYBOARD_CHARACTER_TOGGLE_MASK))
            keyboardShiftFlags&=character^KEYBOARD_CHARACTER_VALUE_MASK;
        return;
    }
    if (attributes&KEYBOARD_ATTRIBUTE_DIRECTION_MASK)
        heldDirectionKeyMasks[(attributes>>KEYBOARD_ATTRIBUTE_GROUP_SHIFT)&KEYBOARD_ATTRIBUTE_GROUP_MASK]
            =attributes>>KEYBOARD_ATTRIBUTE_DIRECTION_SHIFT;
    uint8_t character;character=keyboardUnshiftedCharacters[scan];
    if (character&KEYBOARD_CHARACTER_SPECIAL)
    {
        character&=KEYBOARD_CHARACTER_VALUE_MASK;
        if (!(character&KEYBOARD_CHARACTER_TOGGLE_MASK)) keyboardShiftFlags|=character;
        else if (!((character&KEYBOARD_CHARACTER_CAPS_LOCK) && keyboardCapsLockRequested) && !(attributes&KEYBOARD_ATTRIBUTE_HELD))
            keyboardShiftFlags^=character;
        return;
    }
    if (keyboardShiftFlags&BIOS_SHIFT_CONTROL)
    {character|=ASCII_CONTROL_MARKER;if (attributes&KEYBOARD_ATTRIBUTE_ALPHABETIC) character-=ASCII_CASE_DISTANCE;}
    else if ((keyboardShiftFlags&BIOS_SHIFT_CAPS_LOCK) && (attributes&KEYBOARD_ATTRIBUTE_ALPHABETIC)) character-=ASCII_CASE_DISTANCE;
    else if (keyboardShiftFlags&BIOS_SHIFT_EITHER_SHIFT) character=keyboardShiftedCharacters[scan];
    uint16_t key;key=((uint16_t)scan<<BIOS_KEY_SCAN_SHIFT)|character;
    keyboardLastKey=key;
    /* The original's room test compares the slot after the head with the
     * tail, not the slot after the tail with the head: a key is dropped while
     * exactly one waits, and one more key into 15 leaves the queue empty. The
     * game's rule, kept as it is. */
    uint8_t tail;tail=combatKeyQueueTail;
    if (NextKeySlot(combatKeyQueueHead)!=tail)
    {
        combatKeyQueue[tail]=key;
        tail=NextKeySlot(tail);combatKeyQueueTail=tail;
    }
    if (scan==COMBAT_KEY_D && (keyboardShiftFlags&BIOS_SHIFT_CONTROL)) return;
    if (key==COMBAT_CONTROL_ESCAPE_KEY && (keyboardShiftFlags&BIOS_SHIFT_CONTROL))
    {
        /* Ctrl-Escape: the slot before the tail cleared, the queue emptied. */
        tail=(uint8_t)((tail+COMBAT_KEY_QUEUE_SLOTS-1)%COMBAT_KEY_QUEUE_SLOTS);
        combatKeyQueue[tail]=0;
        combatKeyQueueHead=combatKeyQueueTail;
        BtechRequestTermination();return;
    }
}

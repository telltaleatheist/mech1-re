; MWSNAP.COM: a small resident DOS program that plays a key script into the
; ORIGINAL MechWarrior (1989) and captures the screen and memory at marked
; points, so golden traces can be recorded from the real game under DOSBox-X
; (-silent: no window). Our own code, for studying the original. Its shape
; follows crescent-hawks-rebuild's SNAP.COM (tools/capture/snap.asm there);
; its feeding rules are the port's poll-fed feed (docs/DEBUG_INPUT.md), so a
; script means the same in both and their traces compare.
;
; Usage: MWSNAP K [Thhhhhhhh]
;   K: play the key script in KEYS.BIN (tools/capture/keys.py writes it).
;   T: pin the BIOS clock the campaign's one startup read sees (081B:0C0A,
;      int 1Ah AH=00, mixed into both random words), hex ticks; the port's
;      MECHWARRIOR_DEBUG_BIOS_TICKS. Default 00010000 (the port's default).
;
; KEYS.BIN: a word count, then that many words. Items:
;   0FFFFh          @snap: capture now (MW1Snnnn.BIN, numbered from 0)
;   0FFFEh N        idle:N: N more empty campaign polls (a blocking read skips
;                   it; combat waits at it: idle:0 is where the campaign resumes)
;   0FFF8h W0 W1    random: set the campaign's two random words (MW_MAIN's
;                   DS:03F5 and DS:03F7; campaign only)
;   0FFFBh S        make:S, a scan code pressed (combat only; takes no poll)
;   0FFFAh S        break:S, released (combat only; takes no poll)
;   0FFF9h N        ticks:N: N more combat polls (combat only; ticks:0 marks
;                   where combat starts)
;   otherwise       a key, the BIOS word (scan << 8 | character): in the
;                   campaign it goes into the BIOS buffer; in combat its scan
;                   code is pressed and released, and the next item waits a poll
;
; The campaign (MW.EXE, MW_MAIN.EXE) asks the BIOS for keys: int 16h AH=01
; ("key waiting?") in its wait loops, AH=00 to read. A key goes in only when
; the game waits for one: on a read with the buffer empty, or on the 4th empty
; AH=01 poll since the last key read. The original draws a random number on
; every pass of its wait loop (081B:02FB), so counted polls make every draw
; repeat, whatever the host's speed.
;
; Combat (BTECH.EXE) never asks the BIOS: once a tick its input poll (the
; reference's 0810:2A17) pops the BIOS buffer itself and reads the held-key
; masks its own int 9 handler fills. When BTECH installs that handler (int 21h
; AX=2509h, DS:DX = its 1BF5:2CCB) MWSNAP finds BTECH's load segment, checks
; the poll's first bytes and patches its entry in memory (never the file) with
; a far call here: each poll is counted and fed, then the replaced bytes run.
; Keys reach BTECH's own int 9 handler as from the keyboard: the 8042's D2h
; command puts the scan code in its output buffer and raises IRQ1. Only the
; 1990-01-22 revision (174,533 bytes, the poll at image 2AE1h) is patched; on
; any other bytes MWSNAP writes NOPATCH.TXT and feeds no combat.
;
; A capture: 'MW1S' (4) version 1 (1) video mode (1) CRTC start (2), the
; attribute registers 0-15 (16), the DAC 256 x RGB (768); in mode 0Dh the
; four planes (8000 bytes each, from the CRTC start), then 'MEMS' (4), the
; game's DS (2: MW_MAIN's at its last int 16h call in the campaign, BTECH's at
; its last input poll in combat), and the first
; 640K of memory. The same layout as SNAP's but the magic.
;
; Files it writes in the current directory: MW1Snnnn.BIN, MWSNAPEN.TXT when
; the script is used up, NOPATCH.TXT (BTECH not recognised), KEYFAIL.TXT (a
; combat key the controller didn't deliver), ITEMFAIL.TXT (an item in the
; wrong phase that can't wait, e.g. random: in combat).
;
; Build: nasm -f bin -o MWSNAP.COM mwsnap.asm

        org 100h
        jmp install

POLLS       equ 4
MAX_KEYS    equ 16384
; Owen's BTECH.EXE (1990-01-22): segments relative to its load segment.
INT9_SEG    equ 13f6h                   ; 1BF5 (reference) = 1C06 here
INT9_OFF    equ 2ccbh
POLL_OFF    equ 2ae1h                   ; the reference's 0810:2A17
RANDOM0     equ 03f5h                   ; MW_MAIN's random words
RANDOM1     equ 03f7h

old16       dd 0
old21       dd 0
old1a       dd 0
feeding     db 0
busy        db 0
blocking    db 0
in_combat   db 0
feed_polls  dw 0
feed_wait   dw POLLS
c_polls     dw 0
c_wait      dw 0
game_ds     dw 0                        ; the campaign's, at its last int 16h call
btech_base  dw 0
btech_ds    dw 0                        ; BTECH's DS, at its last input poll
pin_lo      dw 0
pin_hi      dw 1
pin_used    db 0
key_pos     dw 0
key_count   dw 0
count       dw 0
saved_ss    dw 0
saved_sp    dw 0
ret_ip      dw 0
ret_cs      dw 0
fname       db 'MW1S0000.BIN', 0
end_name    db 'MWSNAPEN.TXT', 0
nopatch     db 'NOPATCH.TXT', 0
keyfail     db 'KEYFAIL.TXT', 0
itemfail    db 'ITEMFAIL.TXT', 0
poll_bytes  db 55h, 8bh, 0ech, 83h, 0ech, 0ch, 9ah, 58h, 2fh
header      db 'MW1S', 1
h_mode      db 0
h_start     dw 0
h_attr      times 16 db 0
h_dac       times 768 db 0
header_end:

; ---- the campaign: int 16h ----

; Is the BIOS keyboard buffer empty? ZF set if so. Keeps every register.
buffer_empty:
        push ds
        push ax
        mov ax, 40h
        mov ds, ax
        mov ax, [1ah]
        cmp ax, [1ch]
        pop ax
        pop ds
        ret

int16:
        cmp byte [cs:feeding], 0
        je .chain
        cmp ah, 1
        je .poll
        cmp ah, 11h
        je .poll
        cmp ah, 0
        je .read
        cmp ah, 10h
        jne .chain
.read:
        mov [cs:game_ds], ds
        call buffer_empty
        jne .reset
        mov byte [cs:blocking], 1
        call campaign_feed
        call buffer_empty
        je .chain                       ; nothing fed: the BIOS waits (as the port)
.reset:                                 ; a key is read: both counts start again
        mov word [cs:feed_polls], 0
        mov word [cs:feed_wait], POLLS
        mov word [cs:c_polls], 0
        mov word [cs:c_wait], 0
        jmp .chain
.poll:
        mov [cs:game_ds], ds
        call buffer_empty
        jne .chain
        mov byte [cs:blocking], 0
        call campaign_feed
.chain:
        jmp far [cs:old16]

; Switches to MWSNAP's stack and runs BX's routine with DS = CS. Keeps every
; register and the flags; not re-entered (busy).
on_own_stack:
        pushf
        cmp byte [cs:busy], 0
        jne .busy
        cli
        mov byte [cs:busy], 1
        mov [cs:saved_ss], ss
        mov [cs:saved_sp], sp
        push cs
        pop ss
        mov sp, stack_top
        push ax
        push bx
        push cx
        push dx
        push si
        push di
        push ds
        push es
        push bp
        push cs
        pop ds
        call bx
        pop bp
        pop es
        pop ds
        pop di
        pop si
        pop dx
        pop cx
        pop bx
        pop ax
        mov ss, [cs:saved_ss]
        mov sp, [cs:saved_sp]
        mov byte [cs:busy], 0
.busy:
        popf
        ret

campaign_feed:
        push bx
        mov bx, campaign_items
        call on_own_stack
        pop bx
        ret

; The next item's word in CX (ZF set when the script is used up). DS = CS.
peek:
        mov bx, [key_pos]
        cmp bx, [key_count]
        jae .end
        shl bx, 1
        mov cx, [keys + bx]
        or sp, sp                       ; ZF clear (sp is never 0 here)
        ret
.end:
        cmp bx, bx                      ; ZF set
        ret

; The word after the current item, advancing past it.
take:
        mov bx, [key_pos]
        inc word [key_pos]
        shl bx, 1
        mov ax, [keys + bx]
        ret

campaign_items:
        inc word [feed_polls]
.next:
        call peek
        jz .finished
        cmp byte [blocking], 0
        jne .go
        mov ax, [feed_polls]
        cmp ax, [feed_wait]
        jb .out
.go:
        cmp cx, 0fffbh                  ; make, break, ticks: combat's; wait
        je .out
        cmp cx, 0fffah
        je .out
        cmp cx, 0fff9h
        je .out
        inc word [key_pos]
        cmp cx, 0ffffh
        jne .idle
        mov byte [in_combat], 0
        call capture
        jmp .next
.idle:
        cmp cx, 0fffeh
        jne .random
        call take
        add [feed_wait], ax
        cmp byte [blocking], 0
        jne .next
        jmp .out                        ; the extra polls start now
.random:
        cmp cx, 0fff8h
        jne .key
        call take
        mov dx, ax
        call take
        mov es, [game_ds]
        mov [es:RANDOM0], dx
        mov [es:RANDOM1], ax
        jmp .next
.key:
        mov ah, 5
        pushf
        call far [old16]
        jmp .out
.finished:
        call finished
.out:
        ret

finished:
        cmp byte [feeding], 0
        je .out
        mov byte [feeding], 0
        mov dx, end_name
        call touch
.out:
        ret

; Creates the empty file named at DS:DX.
touch:
        mov ah, 3ch
        xor cx, cx
        int 21h
        jc .out
        mov bx, ax
        mov ah, 3eh
        int 21h
.out:
        ret

; ---- BTECH: found when it installs its int 9 handler; the clock pin ----

int21:
        cmp ax, 2509h
        jne .chain
        cmp dx, INT9_OFF
        jne .chain
        cmp byte [cs:feeding], 0
        je .chain
        push bx
        mov bx, patch_btech
        call on_own_stack               ; DS there is ours: the caller's is saved
        pop bx
.chain:
        jmp far [cs:old21]

; On MWSNAP's stack (see on_own_stack): the caller's DS is 3 words above the return.
patch_btech:
        mov bp, sp
        mov ax, [ss:bp + 6]             ; the caller's DS (return, bp, es, ds)
        sub ax, INT9_SEG
        mov es, ax                      ; BTECH's load segment
        mov si, poll_bytes
        mov di, POLL_OFF
        mov cx, 9
        cld
        repe cmpsb
        jne .refuse
        mov di, POLL_OFF
        mov al, 9ah                     ; call far combat_hook, then a nop
        stosb
        mov ax, combat_hook
        stosw
        mov ax, cs
        stosw
        mov al, 90h
        stosb
        mov ax, es
        mov [btech_base], ax
        mov word [c_polls], 0
        ret
.refuse:
        mov dx, nopatch
        call touch
        ret

int1a:
        cmp ah, 0
        jne .chain
        cmp byte [cs:pin_used], 0
        jne .chain
        mov byte [cs:pin_used], 1
        mov cx, [cs:pin_hi]
        mov dx, [cs:pin_lo]
        xor al, al
        iret
.chain:
        jmp far [cs:old1a]

; ---- combat: BTECH's input poll, patched to call here ----

; Reached by the patched far call at the poll's entry (its return address is
; the nop after it). Feeds, then does the replaced push bp / mov bp,sp /
; sub sp,0Ch on BTECH's stack and goes on at the nop.
combat_hook:
        mov [cs:btech_ds], ds
        push bx
        mov bx, combat_items
        call on_own_stack
        pop bx
        pop word [cs:ret_ip]
        pop word [cs:ret_cs]
        push bp
        mov bp, sp
        sub sp, 0ch
        jmp far [cs:ret_ip]

combat_items:
        inc word [c_polls]
.next:
        call peek
        jz .finished
        mov ax, [c_polls]
        cmp ax, [c_wait]
        jb .out
        cmp cx, 0fffeh                  ; idle: the campaign's; wait
        je .out
        inc word [key_pos]
        cmp cx, 0ffffh
        jne .make
        mov byte [in_combat], 1
        call capture
        mov byte [in_combat], 0
        jmp .next
.make:
        cmp cx, 0fffbh
        jne .break
        call take
        call send_scan
        jmp .next
.break:
        cmp cx, 0fffah
        jne .ticks
        call take
        or al, 80h
        call send_scan
        jmp .next
.ticks:
        cmp cx, 0fff9h
        jne .random
        call take
        mov word [c_polls], 0
        mov [c_wait], ax
        or ax, ax
        jnz .out
        jmp .next
.random:
        cmp cx, 0fff8h
        jne .key
        add word [key_pos], 2           ; campaign only: noted, skipped
        mov dx, itemfail
        call touch
        jmp .next
.key:
        mov al, ch
        push ax
        call send_scan
        pop ax
        or al, 80h
        call send_scan
        mov word [c_polls], 0
        mov word [c_wait], 1
        jmp .out
.finished:
        call finished
.out:
        ret

; Waits (bounded) until the 8042's input buffer is empty. CF set on time-out.
kbc_ready:
        push cx
        mov cx, 0ffffh
.wait:
        in al, 64h
        test al, 2
        jz .ok
        loop .wait
        pop cx
        stc
        ret
.ok:
        pop cx
        clc
        ret

; Sends the scan code in AL as from the keyboard: the 8042's D2h command
; puts it in its output buffer and raises IRQ1, so BTECH's own int 9 handler
; reads it from port 60h. Waits (bounded) until the handler has taken it.
send_scan:
        push cx
        mov ah, al
        call kbc_ready
        jc .fail
        mov al, 0d2h
        out 64h, al
        call kbc_ready
        jc .fail
        mov al, ah
        out 60h, al
        sti                             ; IRQ1 may run now, on our stack
        mov cx, 0ffffh
.taken:
        in al, 64h
        test al, 1
        jz .done
        loop .taken
.fail:
        mov dx, keyfail
        call touch
.done:
        cli
        pop cx
        ret

; ---- a capture (SNAP's, the DS chosen by the phase) ----

capture:
        push cs
        pop ds
        push es
        mov ax, 40h
        mov es, ax
        mov al, [es:49h]
        pop es
        mov [h_mode], al
        mov dx, 3d4h
        in al, dx
        mov bl, al
        mov al, 0ch
        out dx, al
        inc dx
        in al, dx
        mov ah, al
        dec dx
        mov al, 0dh
        out dx, al
        inc dx
        in al, dx
        mov [h_start], ax
        dec dx
        mov al, bl
        out dx, al
        mov dx, 3dah
        in al, dx
        mov dx, 3c0h
        in al, dx
        mov bh, al
        xor cx, cx
        mov si, h_attr
.attr:
        mov dx, 3dah
        in al, dx
        mov dx, 3c0h
        mov al, cl
        or al, 20h
        out dx, al
        inc dx
        in al, dx
        mov [si], al
        inc si
        inc cl
        cmp cl, 16
        jb .attr
        mov dx, 3dah
        in al, dx
        mov dx, 3c0h
        mov al, bh
        out dx, al
        mov dx, 3c7h
        xor al, al
        out dx, al
        mov dx, 3c9h
        mov si, h_dac
        mov cx, 768
.dac:
        in al, dx
        mov [si], al
        inc si
        loop .dac
        mov ax, [count]
        mov di, fname + 7
        mov cx, 4
        mov bx, 10
.digit:
        xor dx, dx
        div bx
        add dl, '0'
        mov [di], dl
        dec di
        loop .digit
        mov ah, 3ch
        xor cx, cx
        mov dx, fname
        int 21h
        jc .done
        mov bx, ax
        mov ah, 40h
        mov cx, header_end - header
        mov dx, header
        int 21h
        cmp byte [h_mode], 0dh
        jne .close
        mov dx, 3ceh
        in al, dx
        mov [gc_index], al
        mov al, 4
        out dx, al
        inc dx
        in al, dx
        mov [gc_map], al
        dec dx
        mov al, 5
        out dx, al
        inc dx
        in al, dx
        mov [gc_mode], al
        and al, 0f7h
        out dx, al
        xor cl, cl
.plane:
        mov dx, 3ceh
        mov al, 4
        out dx, al
        inc dx
        mov al, cl
        out dx, al
        push cx
        push ds
        mov ax, 0a000h
        mov ds, ax
        mov dx, [cs:h_start]
        mov cx, 8000
        mov ah, 40h
        int 21h
        pop ds
        pop cx
        inc cl
        cmp cl, 4
        jb .plane
        mov dx, 3ceh
        mov al, 5
        out dx, al
        inc dx
        mov al, [gc_mode]
        out dx, al
        dec dx
        mov al, 4
        out dx, al
        inc dx
        mov al, [gc_map]
        out dx, al
        dec dx
        mov al, [gc_index]
        out dx, al
        mov ax, [game_ds]
        cmp byte [in_combat], 0
        je .ds
        mov ax, [btech_ds]
.ds:
        mov [mem_ds], ax
        mov cx, 6
        mov dx, mem_tag
        mov ah, 40h
        int 21h
        xor si, si
.mem:
        push ds
        mov ds, si
        xor dx, dx
        mov cx, 8000h
        mov ah, 40h
        int 21h
        pop ds
        add si, 800h
        cmp si, 0a000h
        jb .mem
.close:
        mov ah, 3eh
        int 21h
        inc word [count]
.done:
        ret

mem_tag     db 'MEMS'
mem_ds      dw 0
gc_index    db 0
gc_map      db 0
gc_mode     db 0

            times 768 db 0
stack_top:

keys        times MAX_KEYS dw 0

resident_end:

; ---- installation ----

install:
        mov si, 81h
.parse:
        lodsb
        cmp al, 13
        je .parsed
        or al, 20h
        cmp al, 'k'
        je .flag
        cmp al, 't'
        jne .parse
        xor dx, dx                      ; T: hex ticks into DX:BX
        xor bx, bx
.hex:
        mov al, [si]
        sub al, '0'
        cmp al, 9
        jbe .digit
        mov al, [si]
        or al, 20h
        sub al, 'a'
        cmp al, 5
        ja .hexdone
        add al, 10
.digit:
        inc si
        mov cx, 4
.shift:
        shl bx, 1
        rcl dx, 1
        loop .shift
        or bl, al
        jmp .hex
.hexdone:
        mov [pin_lo], bx
        mov [pin_hi], dx
        jmp .parse
.flag:
        mov byte [want_keys], 1
        jmp .parse
.parsed:
        cmp byte [want_keys], 0
        je .bad_keys
        mov ax, 3d00h
        mov dx, keys_name
        int 21h
        jc .bad_keys
        mov bx, ax
        mov ah, 3fh
        mov cx, 2
        mov dx, key_count
        int 21h
        jc .bad_keys
        mov cx, [key_count]
        cmp cx, MAX_KEYS
        ja .bad_keys
        shl cx, 1
        mov ah, 3fh
        mov dx, keys
        int 21h
        jc .bad_keys
        mov ah, 3eh
        int 21h
        mov ax, 3516h
        int 21h
        mov [old16], bx
        mov [old16+2], es
        mov ax, 2516h
        mov dx, int16
        int 21h
        mov ax, 3521h
        int 21h
        mov [old21], bx
        mov [old21+2], es
        mov ax, 2521h
        mov dx, int21
        int 21h
        mov ax, 351ah
        int 21h
        mov [old1a], bx
        mov [old1a+2], es
        mov ax, 251ah
        mov dx, int1a
        int 21h
        mov byte [feeding], 1
        mov ah, 9
        mov dx, msg
        int 21h
        mov dx, resident_end + 15
        mov cl, 4
        shr dx, cl
        mov ax, 3100h
        int 21h
.bad_keys:
        mov ah, 9
        mov dx, bad_msg
        int 21h
        mov ax, 4c01h
        int 21h

want_keys db 0
keys_name db 'KEYS.BIN', 0
msg     db 'MWSNAP installed', 13, 10, '$'
bad_msg db 'MWSNAP: needs K and a readable KEYS.BIN', 13, 10, '$'

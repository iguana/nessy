; neslib.s - NES library implementation for cc65
; Implements functions declared in neslib.h

.import popa, popax
.importzp _nmi_flag, ppu_ctrl_var, ppu_mask_var, nmi_ready
.importzp scroll_x, scroll_y, pad_state
.import pal_buf, pal_dirty, oam_buf

.export _ppu_wait_nmi
.export _ppu_on_bg, _ppu_on_spr, _ppu_on_all, _ppu_off
.export _ppu_mask
.export _vram_adr, _vram_put, _vram_write, _vram_fill
.export _pal_all, _pal_bg, _pal_spr, _pal_col
.export _pad_poll
.export _scroll

; Temp zero-page pointers for indirect addressing
.segment "ZEROPAGE"
tmp_ptr:   .res 2
tmp_len:   .res 2
tmp_val:   .res 1

.segment "CODE"

; ────────────────────────────────────────────────
; void __fastcall__ ppu_wait_nmi(void)
; ────────────────────────────────────────────────
_ppu_wait_nmi:
    lda #$00
    sta _nmi_flag
@wait:
    lda _nmi_flag
    beq @wait
    rts

; ────────────────────────────────────────────────
; void __fastcall__ ppu_on_bg(void)
; ────────────────────────────────────────────────
_ppu_on_bg:
    lda ppu_mask_var
    ora #$08
    sta ppu_mask_var
    lda ppu_ctrl_var
    ora #$80
    sta ppu_ctrl_var
    sta $2000
    rts

; ────────────────────────────────────────────────
; void __fastcall__ ppu_on_spr(void)
; ────────────────────────────────────────────────
_ppu_on_spr:
    lda ppu_mask_var
    ora #$10
    sta ppu_mask_var
    lda ppu_ctrl_var
    ora #$80
    sta ppu_ctrl_var
    sta $2000
    rts

; ────────────────────────────────────────────────
; void __fastcall__ ppu_on_all(void)
; ────────────────────────────────────────────────
_ppu_on_all:
    lda ppu_mask_var
    ora #$18
    sta ppu_mask_var
    lda ppu_ctrl_var
    ora #$80
    sta ppu_ctrl_var
    sta $2000
    rts

; ────────────────────────────────────────────────
; void __fastcall__ ppu_off(void)
; ────────────────────────────────────────────────
_ppu_off:
    lda ppu_mask_var
    and #$E7             ; ~$18 = clear show BG + sprites
    sta ppu_mask_var
    sta $2001
    rts

; ────────────────────────────────────────────────
; void __fastcall__ ppu_mask(unsigned char mask)
; ────────────────────────────────────────────────
_ppu_mask:
    sta ppu_mask_var
    rts

; ────────────────────────────────────────────────
; void __fastcall__ vram_adr(unsigned int adr)
; A=low, X=high
; ────────────────────────────────────────────────
_vram_adr:
    stx $2006
    sta $2006
    rts

; ────────────────────────────────────────────────
; void __fastcall__ vram_put(unsigned char val)
; ────────────────────────────────────────────────
_vram_put:
    sta $2007
    rts

; ────────────────────────────────────────────────
; void __fastcall__ vram_write(const unsigned char *data, unsigned int len)
; fastcall: len in A/X, data on C stack
; ────────────────────────────────────────────────
_vram_write:
    sta tmp_len
    stx tmp_len+1
    jsr popax
    sta tmp_ptr
    stx tmp_ptr+1

    ldy #$00
@loop:
    lda tmp_len
    ora tmp_len+1
    beq @done

    lda (tmp_ptr),y
    sta $2007

    inc tmp_ptr
    bne :+
    inc tmp_ptr+1
:
    lda tmp_len
    bne :+
    dec tmp_len+1
:   dec tmp_len

    jmp @loop
@done:
    rts

; ────────────────────────────────────────────────
; void __fastcall__ vram_fill(unsigned char val, unsigned int len)
; fastcall: len in A/X, val on C stack
; ────────────────────────────────────────────────
_vram_fill:
    sta tmp_len
    stx tmp_len+1
    jsr popa
    sta tmp_val

@loop:
    lda tmp_len
    ora tmp_len+1
    beq @done

    lda tmp_val
    sta $2007

    lda tmp_len
    bne :+
    dec tmp_len+1
:   dec tmp_len

    jmp @loop
@done:
    rts

; ────────────────────────────────────────────────
; void __fastcall__ pal_all(const unsigned char *data)
; A=low, X=high
; ────────────────────────────────────────────────
_pal_all:
    sta tmp_ptr
    stx tmp_ptr+1
    ldy #$00
@loop:
    lda (tmp_ptr),y
    sta pal_buf,y
    iny
    cpy #$20
    bne @loop
    lda #$01
    sta pal_dirty
    rts

; ────────────────────────────────────────────────
; void __fastcall__ pal_bg(const unsigned char *data)
; A=low, X=high
; ────────────────────────────────────────────────
_pal_bg:
    sta tmp_ptr
    stx tmp_ptr+1
    ldy #$00
@loop:
    lda (tmp_ptr),y
    sta pal_buf,y
    iny
    cpy #$10
    bne @loop
    lda #$01
    sta pal_dirty
    rts

; ────────────────────────────────────────────────
; void __fastcall__ pal_spr(const unsigned char *data)
; A=low, X=high
; ────────────────────────────────────────────────
_pal_spr:
    sta tmp_ptr
    stx tmp_ptr+1
    ldy #$00
@loop:
    lda (tmp_ptr),y
    sta pal_buf+16,y
    iny
    cpy #$10
    bne @loop
    lda #$01
    sta pal_dirty
    rts

; ────────────────────────────────────────────────
; void __fastcall__ pal_col(unsigned char index, unsigned char color)
; fastcall: color in A, index on C stack
; ────────────────────────────────────────────────
_pal_col:
    sta tmp_val
    jsr popa
    tax
    lda tmp_val
    sta pal_buf,x
    lda #$01
    sta pal_dirty
    rts

; ────────────────────────────────────────────────
; unsigned char __fastcall__ pad_poll(unsigned char pad)
; A = pad number (0 or 1)
; ────────────────────────────────────────────────
_pad_poll:
    tax
    lda #$01
    sta $4016
    lda #$00
    sta $4016

    cpx #$00
    beq @port0

    ldy #$08
    lda #$00
@read1:
    pha
    lda $4017
    and #$01
    lsr
    pla
    rol
    dey
    bne @read1
    sta pad_state+1
    rts

@port0:
    ldy #$08
    lda #$00
@read0:
    pha
    lda $4016
    and #$01
    lsr
    pla
    rol
    dey
    bne @read0
    sta pad_state
    rts

; ────────────────────────────────────────────────
; void __fastcall__ scroll(unsigned int x, unsigned int y)
; fastcall: y in A/X, x on C stack
; ────────────────────────────────────────────────
_scroll:
    sta scroll_y
    jsr popax
    sta scroll_x
    rts

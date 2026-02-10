; crt0.s - NES startup code for cc65
; iNES header, reset/NMI/IRQ handlers, vector table

.import _main
.import __DATA_LOAD__, __DATA_RUN__, __DATA_SIZE__
.importzp sp

.export __STARTUP__: absolute = 1
.exportzp _nmi_flag

.segment "HEADER"
; iNES header (16 bytes)
.byte "NES", $1A       ; Magic number
.byte $01              ; 1 x 16KB PRG-ROM
.byte $01              ; 1 x 8KB CHR-ROM
.byte $01              ; Mapper 0 (NROM), vertical mirroring
.byte $00              ; Mapper 0 upper nybble
.byte $00,$00,$00,$00  ; Unused
.byte $00,$00,$00,$00  ; Unused

.segment "ZEROPAGE"

_nmi_flag:      .res 1  ; Set by NMI handler, cleared by ppu_wait_nmi
nmi_ready:      .res 1  ; NMI rendering control: 0=skip, 1=do PPU updates
ppu_ctrl_var:   .res 1  ; Shadow of PPU_CTRL ($2000)
ppu_mask_var:   .res 1  ; Shadow of PPU_MASK ($2001)
scroll_x:      .res 1
scroll_y:      .res 1
pad_state:     .res 2   ; Controller state (2 pads)

.exportzp ppu_ctrl_var, ppu_mask_var, nmi_ready
.exportzp scroll_x, scroll_y, pad_state

.segment "OAM"
oam_buf:    .res 256     ; Sprite OAM buffer at $0200

.segment "BSS"
pal_buf:    .res 32      ; Palette buffer
pal_dirty:  .res 1       ; Non-zero = upload palette in NMI
.export pal_buf, pal_dirty, oam_buf

.segment "STARTUP"

; ────────────────────────────────────────────────
; RESET handler
; ────────────────────────────────────────────────
reset:
    sei                  ; Disable IRQs
    cld                  ; Disable decimal mode (not used on NES but good practice)
    ldx #$40
    stx $4017            ; Disable APU frame IRQ
    ldx #$FF
    txs                  ; Set stack pointer to $01FF
    inx                  ; X = 0
    stx $2000            ; Disable NMI
    stx $2001            ; Disable rendering
    stx $4010            ; Disable DMC IRQs

    ; Wait for first vblank
@vblank1:
    bit $2002
    bpl @vblank1

    ; Clear RAM ($0000-$07FF)
    lda #$00
    ldx #$00
@clear_ram:
    sta $0000,x
    sta $0100,x
    sta $0200,x
    sta $0300,x
    sta $0400,x
    sta $0500,x
    sta $0600,x
    sta $0700,x
    inx
    bne @clear_ram

    ; Fill OAM buffer with $FF (hide all sprites off-screen)
    lda #$FF
    ldx #$00
@clear_oam:
    sta oam_buf,x
    inx
    bne @clear_oam

    ; Wait for second vblank (PPU is now stable)
@vblank2:
    bit $2002
    bpl @vblank2

    ; Copy DATA segment from ROM to RAM
    ldx #<__DATA_SIZE__
    beq @data_done
    ldy #$00
@copy_data:
    lda __DATA_LOAD__,y
    sta __DATA_RUN__,y
    iny
    dex
    bne @copy_data
@data_done:

    ; Initialize palette to all black
    lda #$0F
    ldx #$00
@init_pal:
    sta pal_buf,x
    inx
    cpx #$20
    bne @init_pal

    ; Initialize cc65 C software stack pointer (grows down from top of RAM)
    lda #$00
    sta sp
    lda #$08
    sta sp+1             ; sp = $0800 (top of NES work RAM)

    ; Mark NMI as ready
    lda #$01
    sta nmi_ready

    ; Jump to C main()
    jmp _main

; ────────────────────────────────────────────────
; NMI handler (called every vblank)
; ────────────────────────────────────────────────
.segment "CODE"

nmi:
    pha
    txa
    pha
    tya
    pha

    ; Set NMI flag
    lda #$01
    sta _nmi_flag

    ; Check if we should do PPU updates
    lda nmi_ready
    beq @nmi_done

    ; OAM DMA
    lda #$00
    sta $2003            ; OAM address = 0
    lda #>oam_buf
    sta $4014            ; Trigger OAM DMA from $0200

    ; Upload palette if dirty
    lda pal_dirty
    beq @no_pal

    lda #$3F
    sta $2006
    lda #$00
    sta $2006

    ldx #$00
@pal_loop:
    lda pal_buf,x
    sta $2007
    inx
    cpx #$20
    bne @pal_loop

    lda #$00
    sta pal_dirty
@no_pal:

    ; Apply scroll
    lda scroll_x
    sta $2005
    lda scroll_y
    sta $2005

    ; Apply PPU_CTRL and PPU_MASK
    lda ppu_ctrl_var
    sta $2000
    lda ppu_mask_var
    sta $2001

@nmi_done:
    pla
    tay
    pla
    tax
    pla
    rti

; ────────────────────────────────────────────────
; IRQ handler
; ────────────────────────────────────────────────
irq:
    rti

; ────────────────────────────────────────────────
; Vector table
; ────────────────────────────────────────────────
.segment "VECTORS"
.word nmi       ; $FFFA - NMI vector
.word reset     ; $FFFC - RESET vector
.word irq       ; $FFFE - IRQ vector

; ────────────────────────────────────────────────
; CHR-ROM data
; ────────────────────────────────────────────────
.segment "CHARS"
.incbin "../chr/ascii.chr"

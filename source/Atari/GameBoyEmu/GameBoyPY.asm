/* Gameboy Demo ROM for use with UnoCart
 * by Mark Keates/Wrathchild@AtariAge
 * This file builds with WUDSN/MADS into an 8K Atari ROM
 * The 8k ROM is loaded with the GameBoy/VRAM from Pokemon:
 */

CART_CMD_REFRESH_SCREEN = $30
CART_CMD_INIT_VRAM = $31
CART_CMD_PREPARE_LINE = $31

GAMEBOY_FONT = $9C00
GAMEBOY_OAM = $9E00
GAMEBOY_REGS = $9F40

LCDCONT = $9F40
LCDSTAT = $9F41
SCROLLY = $9F42
SCROLLX = $9F43
CURLINE = $9F44
CMPLINE = $9F45
DMACONT = $9F46
BGRDPAL = $9F47
OBJ0PAL = $9F48
OBJ1PAL = $9F49
WNDPOSY = $9F4A
WNDPOSX = $9F4B

;@com.wudsn.ide.asm.outputfileextension=.rom

CARTFG_DIAGNOSTIC_CART = $80       ;Flag value: Directly jump via CARTAD during RESET.
CARTFG_START_CART      = $04       ;Flag value: Jump via CARTAD and then via CARTCS.
CARTFG_BOOT            = $01       ;Flag value: Boot peripherals, then start the module.

SETVBV = $E45C
XITVBV = $E462
COLDSV = $E477				; Coldstart (powerup) entry point
WARMSV = $E474				; Warmstart entry point

COLBAK = $D01A
CONSOL = $D01F
PORTB = $D301
DMACTL = $D400
HSCROL = $D404
PMBASE = $D407
NMIEN = $D40E

SDMCTL = $22F
GPRIOR = $26F
PCOLR0 = $2C0
PCOLR1 = $2C1
PCOLR2 = $2C2
PCOLR3 = $2C3
COLOR0 = $2C4
COLOR1 = $2C5
COLOR2 = $2C6
COLOR3 = $2C7
COLOR4 = $2C8
CHBAS = $2F4

; ************************ VARIABLES ****************************

DOSINI		equ $0C
MEMLO		equ $02E7
RunVec		equ $02E0
IniVec		equ $02E2
VCOUNT		equ $D40B
WSYNC		equ $D40A
GINTLK		equ $03FA
TRIG3		equ $D013
SDLSTL		equ $230
CIOV		equ $E456

scroll_dir = $D0
scroll_delay = $D1
scroll_x = $D2
scroll_y = $D3

; ************************ CODE ****************************


	opt h-                     ;Disable Atari COM/XEX file headers

	org $a000                  ;RD5 cartridge base
	opt f+                     ;Activate fill mode
load_vram
	ins "pokemon_vram.xex", 6
	
	org $aff7
;Cartridge initalization
;Only the minimum of the OS initialization is complete, you don't want to code here normally.
init    .proc
	lda #$34
	sta COLOR4
	sta COLBAK
	rts
	.endp ; proc init
	
	org $b000
;Cartridge start
;Copy main code to RAM and transfer control there
start   .proc
	lda #$84
	sta COLOR4
	sta COLBAK
	jsr copy_wait_for_cart
	jsr copy_page_6_data
	jsr copy_display_list
	jmp load_vram
	.endp
	
.proc	copy_wait_for_cart
	ldy #0
@
	lda wait_for_cart,y
	sta .ADR wait_for_cart,y
	iny
	bne @-
@	lda wait_for_cart+$100,y
	sta .ADR wait_for_cart+$100,y
	iny
	cpy #(.len[wait_for_cart] & 0xFF)
	bne @-
	rts
	.endp

.proc	copy_page_6_data
	ldy #0
@
	lda page_6_data_start,y
	sta .ADR page_6_data_start,y
	iny
	cpy #page_6_data_end-page_6_data_start
	bne @-
	rts
	.endp

.proc	copy_display_list
	ldy #0
@
	lda display_list,y
	sta .ADR display_list,y
	lda display_list+$100,y
	sta .ADR display_list+$100,y
	iny
	bne @-
@
	lda display_list+$200,y
	sta .ADR display_list+$200,y
	iny
	cpy #(.len[display_list] & 0xFF)
	bne @-
	ldy #0
@
	lda G15_dlist,y
	sta .ADR G15_dlist,y
	iny
	cpy #.len[G15_dlist]
	bne @-
	rts
	.endp

.macro wait_cart_ready
	lda #'X'
@
	cmp $D5DE
	; wait for cart to become ready
	bne @-
	.endm

	.align $ba00,$FF
	org $ba00, $700

FLOWER_ANIM_ADDR = $9030
WATER_ANIM_ADDR = $9140

; cmd is in Accumulator
.proc wait_for_cart
	wait_cart_ready
	
	mva #$1A COLOR0
	mva #$16 COLOR1
	mva #$12 COLOR2
	sta COLOR4
	mva #$00 COLOR3 ; unused

	jsr .ADR wait_for_vbi
	lda SDMCTL
	pha
	lda #0
	sta NMIEN
	sta SDMCTL
	sta DMACTL

	; init vram (copy Pokemon data)
	mva #CART_CMD_INIT_VRAM $D5DF
	wait_cart_ready
	
	; draw the initial screen
	mva #CART_CMD_REFRESH_SCREEN $D5DF
	wait_cart_ready
	
	lda #$1E
	jsr .ADR set_bg_colour
	
	lda $BFFD
	cmp #CARTFG_START_CART
	bne CartOK
	; show green if boot rom still present
	lda #$CA
	jsr .ADR set_bg_colour

CartOK
	; copy OAM / sprite data
	ldx #(example_regs - example_oam)/4
	stx $D541 ; # of sprites
	ldy #0
	sty $D542 ; # all lines scroll
@	lda .ADR example_oam,y
	sta GAMEBOY_OAM,y
	iny
	lda .ADR example_oam,y
	sta GAMEBOY_OAM,y
	iny
	lda .ADR example_oam,y
	sta GAMEBOY_OAM,y
	iny
	lda .ADR example_oam,y
	sta GAMEBOY_OAM,y
	iny
	dex
	bne @-
	; copy example register set
	ldy #0
@	lda .ADR example_regs,y
	sta GAMEBOY_REGS,y
	iny
	cpy #12
	bne @-

	jsr .ADR init_scroll
	
	; each VBI will ask for a redraw
	LDA #7
	LDX #>(.ADR VBI)
	LDY #<(.ADR VBI)
	JSR SETVBV

	; clear up old dlist
	ldy #$7F
	lda #0
@	sta GAMEBOY_FONT,y
	dey
	bpl @-
	ldy #7
@	lda #$FF
	sta GAMEBOY_FONT+32*8,y
	lda #$AA
	sta GAMEBOY_FONT+34*8,y
	lda #$55
	sta GAMEBOY_FONT+36*8,y
	dey
	bpl @-
	
	; Restore DMA (screen turns on at next VBlank)
	pla
	sta SDMCTL
	
endless
	; enable dli's
	mva #$C0 NMIEN
	ldy #0
anim_loop
	; dlist toggler
	lda #5
	cmp CONSOL
	beq @+
	ldx #>(.ADR display_list)
	lda #<(.ADR display_list)
	beq @+1
	bne @+1
@	ldx #>(.ADR G15_dlist)
	lda #<(.ADR G15_dlist)
@	stx $231
	sta $230
	; delay
	lda #15
	jsr .ADR wait_n_vbi
	; flower
	tya
	pha
	and #3
:4	asl
	tax
	ldy #0
flower_loop
	lda .ADR flower_images,x
	sta FLOWER_ANIM_ADDR,y
	inx
	iny
	cpy #16
	bne flower_loop
	; water
	ldx #0
	pla
	tay
	and #4
	bne	r_left
r_right
	lda WATER_ANIM_ADDR,x
	lsr
	ror WATER_ANIM_ADDR,x
	lda WATER_ANIM_ADDR+1,x
	lsr
	ror WATER_ANIM_ADDR+1,x
	inx
	inx
	cpx #16
	bne r_right
	beq r_done
r_left
	lda WATER_ANIM_ADDR,x
	asl
	rol WATER_ANIM_ADDR,x
	lda WATER_ANIM_ADDR+1,x
	asl
	rol WATER_ANIM_ADDR+1,x
	inx
	inx
	cpx #16
	bne r_left
r_done
	iny
	cpy #8
	bne anim_loop
	jmp .ADR endless

wait_for_vbi
	lda #1
wait_n_vbi
	clc
	adc $14
@
	cmp $14
	bne @-
	rts

set_bg_colour
	sta COLOR4
	sta COLBAK
	rts

init_scroll
	mva #1 scroll_delay
	mva #3 scroll_dir
	mva #96 scroll_x
	mva #96*2 scroll_y
	rts

	; scroller
do_scroll
	lda #6
	cmp CONSOL
	beq scroll_done
	dec scroll_delay
	bne scroll_done
	mva #5 scroll_delay
	lda scroll_dir
	cmp #1
	bne not_right
	; scroll right
	inc scroll_x
	lda scroll_x
	cmp #96
	bne @+
	lda #2
	sta scroll_dir
@	bne scroll_done
not_right
	cmp #2
	bne not_down
	; scroll down
	inc scroll_y
	lda scroll_y
	cmp #96*2
	bne @+
	lda #3
	sta scroll_dir
@	bne scroll_done
not_down
	cmp #3
	bne not_left
	; scroll left
	dec scroll_x
	bne @+
	lda #4
	sta scroll_dir
@	bne scroll_done
not_left
	cmp #4
	bne scroll_done
	; scroll up
	dec scroll_y
	bne scroll_done
	lda #1
	sta scroll_dir
scroll_done
	lda scroll_x
	sta SCROLLX
	lda scroll_y 
	lsr 
	sta SCROLLY
	rts

VBI
	mva #$0E COLBAK ; white
	jsr .ADR do_scroll
	mva #CART_CMD_REFRESH_SCREEN $D5DF
	wait_cart_ready
	mva $D540 HSCROL
	mva COLOR4 COLBAK
	JMP XITVBV

	.endp

	.align $bc00,$FF
	org $bc00, $600

page_6_data_start
flower_images
	.byte $81, $00, $00, $18, $00, $24, $85, $5A, $1C, $42, $18, $A5, $00, $7E, $81, $18 ; down
	.byte $81, $00, $00, $18, $00, $24, $85, $5A, $1C, $42, $18, $A5, $00, $7E, $81, $18 ; down
	.byte $81, $00, $00, $0C, $00, $12, $82, $2D, $0E, $E1, $0C, $73, $00, $3E, $81, $18 ; right
	.byte $81, $18, $00, $24, $04, $5a, $9d, $42, $18, $24, $00, $DB, $00, $7E, $81, $18 ; up

example_oam
	.byte $28, $19, $00, $00
	.byte $28, $21, $01, $00
	.byte $30, $19, $02, $00
	.byte $30, $21, $03, $00
	.byte $4C, $3A, $34, $00
	.byte $4C, $42, $35, $00
	.byte $54, $3A, $36, $00
	.byte $54, $42, $37, $00
	.byte $6C, $53, $44, $20
	.byte $6C, $4B, $45, $20
	.byte $74, $53, $46, $20
	.byte $74, $4B, $47, $20

example_regs
	.byte $E3
	.byte $89
	.byte $60 ; X = 96
	.byte $60 ; Y = 96
	.byte $90
	.byte $00
	.byte $C3
	.byte $E4 ; 11 10 01 00 - 3 2 1 0
	.byte $D0 ; 11 01 00 00 - 3 1 0 0
	.byte $E0 ; 11 10 00 00 - 3 2 0 0
	.byte $90
	.byte $07
page_6_data_end

	.align $bd00,$FF
	org $bd00,$7c00 ;Dlist
G15_dlist
:3	.byte $70
	.byte $5E
	.word $A010
:84	.byte $1E
	.byte $5E
	.word $B000
:74	.byte $1E
	.byte $41
	.word .ADR G15_dlist

	.align $be00,$F8
	org $be00,$7d80 ;Dlist
.proc display_list
:3	.byte $70
:160	.byte $5E, $00, $D5
	.byte $41
	.word .ADR display_list
	.endp
	
; ************************ CARTRIDGE CONTROL BLOCK *****************

	.align $bff8,$FF
	org $bff8                 ;Cartridge control block
	.byte 'G', 'B'		  ;Signal to the UNO Cart for VRAM support
	.word start               ;CARTCS
	.byte 0                   ;CART
	.byte CARTFG_START_CART   ;CARTFG
	.word init                ;CARTAD

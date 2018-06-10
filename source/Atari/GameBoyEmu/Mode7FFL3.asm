/* Mode-7 Demo ROM for use with UnoCart
 * by Mark Keates/Wrathchild@AtariAge
 * This file builds with WUDSN/MADS into an 8K Atari ROM
 * The 8k ROM is loaded with the Final Fantasy/VRAM from Pokemon:
 */

CART_CMD_REFRESH_M7_SCREEN = $30
CART_CMD_INIT_M7_VRAM = $31
CART_CMD_PROCESS_MOVEMENT = $32

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

; ************************ CODE ****************************


	opt h-                     ;Disable Atari COM/XEX file headers

	org $a000                  ;RD5 cartridge base
	opt f+                     ;Activate fill mode
load_vram
	ins "FFL3_vram.xex", 6
	
	org $b1c0
;Cartridge initalization
;Only the minimum of the OS initialization is complete, you don't want to code here normally.
init    .proc
	lda #$34
	sta COLOR4
	sta COLBAK
	rts
	.endp ; proc init
	
	org $b200
;Cartridge start
;Copy main code to RAM and transfer control there
start   .proc
	lda #$84
	sta COLOR4
	sta COLBAK
	jsr copy_wait_for_cart
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

.proc	copy_display_list
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
@	lda $D5DE
	lsr ; a
	lsr ; a
	lsr ; a
	lsr ; a
	tay
	lda .ADR Hex2Screen,y
	sta $6FE
@	lda $D5DE
	and #$f
	tay
	lda .ADR Hex2Screen,y
	sta $6FF
	lda $D5DE
	cmp #'X'
	; wait for cart to become ready
	bne @-
	.endm

.macro show_msg msg
	ldy #<(.ADR :msg)
	ldx #>(.ADR :msg)
	jsr .ADR show_text
	.endm

	.align $ba00,$FF
	org $ba00, $700

; cmd is in Accumulator
.proc wait_for_cart
	mva #$E4 COLOR4
	show_msg MsgWaitCart
	; dlist toggler
	ldx #>(.ADR G15_dlist)
	lda #<(.ADR G15_dlist)
	stx $231
	sta $230
	jsr .ADR wait_for_vbi

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

	; init vram (copy VRAM data)
	mva #CART_CMD_INIT_M7_VRAM $D5DF
	show_msg MsgInit
	wait_cart_ready
	
	; draw the initial screen
	mva #CART_CMD_REFRESH_M7_SCREEN $D5DF
	show_msg MsgRefresh
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
	show_msg MsgOk
	; each VBI will ask for a redraw
	LDA #7
	LDX #>(.ADR VBI)
	LDY #<(.ADR VBI)
	JSR SETVBV

	; clear up old dlist
	ldy #$7F
	lda #0
@	sta $9C00,y
	dey
	bpl @-
	
	; Restore DMA (screen turns on at next VBlank)
	pla
	sta SDMCTL
	; enable dli's
	mva #$C0 NMIEN
	
endless
	ldy #0
anim_loop
	; delay
	lda #15
	jsr .ADR wait_n_vbi
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

clear_text
	ldy #$27
	lda #0
@	sta $6D8,y
	dey
	bpl @-
	rts

show_text
	sty .ADR st + 1
	stx .ADR st + 2
	ldy #0
st:	lda $ffff,y
	bmi @+
	sta $6D8,y
	iny
	bpl st
@	rts

VBI
	mva #$0E COLBAK ; white
	mva #CART_CMD_REFRESH_M7_SCREEN $D5DF
	wait_cart_ready
	mva COLOR4 COLBAK
	JMP XITVBV

MsgWaitCart
	dta d'Waiting for Cartridge',$80
MsgInit
	dta d'Initialising VRAM',$80
MsgRefresh
	dta d'First VRAM Refresh',$80
MsgOk
	dta d'OK',$80
Hex2Screen
	dta d'0123456789ABCDEF'

	.endp

	.align $bd00,$FF
	org $bd00,$600 ;Dlist
.proc G15_dlist
:3	.byte $70
	.byte $42
	.word $6D8
	.byte $4E
	.word $A010
:84	.byte $1E
	.byte $4E
	.word $B000
:74	.byte $1E
	.byte $41
	.word .ADR G15_dlist
.endp

; ************************ CARTRIDGE CONTROL BLOCK *****************

	.align $bff8,$FF
	org $bff8                 ;Cartridge control block
	.byte 'M', '7'		  ;Signal to the UNO Cart for MODE-7 VRAM support
	.word start               ;CARTCS
	.byte 0                   ;CART
	.byte CARTFG_START_CART   ;CARTFG
	.word init                ;CARTAD

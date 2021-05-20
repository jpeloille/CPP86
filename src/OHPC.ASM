ideal
p8086
model   tiny
warn

;---------------------------------------------------------------------------------------------------
; Macros
;---------------------------------------------------------------------------------------------------
; Pad code to create entry point at specified address
macro   entry   addr
	pad = str_banner - $ + addr - 0E000h
	if pad lt 0
		err     'No room for ENTRY point'
	endif
	if pad gt 0
		db      pad dup(0FFh)
	endif
endm

; Force far jump
macro   jmpf    segm, offs
	db      0EAh;
	dw      offs, segm
endm

; Line feed and carriage return
LF      equ     0Ah
CR      equ     0Dh


;---------------------------------------------------------------------------------------------------
; ROM BIOS
;---------------------------------------------------------------------------------------------------
assume ds:code, ss:code, cs:code, es:code
segment code
	org     0E000h                          ; 8K ROM BIOS starts at F000:E0000


str_banner      db      'Turbo XT BIOS for 8088/V20', 0
str_banner_2    db      CR, LF, 'Additions by Ya`akov Miles (1987) and Jon Petrosky (2008)', CR, LF, 0
dix             db      10

;---------------------------------------------------------------------------------------------------
; BIOS Power-On Self Test (POST)
;---------------------------------------------------------------------------------------------------
	entry   0E05Bh                          ; IBM restart entry point
proc    post    near

cold_boot:
	mov     ax, cs                        ; Entered by POWER_ON/RESET
	mov     ds, ax

inst_adc:
	mov     al, -0Ah
	add     [dix], al


;--------------------------------------------------------------------------------------------------
; Power-On Entry Point
;--------------------------------------------------------------------------------------------------
	entry   0FFF0h                          ; Hardware power reset entry
proc    power   far                             ;   CPU begins here on power up

	jmpf    0F000h, cold_boot

endp    power


;--------------------------------------------------------------------------------------------------
; BIOS Release Date and Signature
;--------------------------------------------------------------------------------------------------
	entry   0FFF5h
	db      "04/23/08"                      ; Release date (MM/DD/YY)
						;   originally 08/23/87
	entry   0FFFEh
	db      0FEh                            ; Computer type (XT)
;       db      0                               ; Checksum byte (8K ROM must sum to 0 mod 256)

ends    code

end

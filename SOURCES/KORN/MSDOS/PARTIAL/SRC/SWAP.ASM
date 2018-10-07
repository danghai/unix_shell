	TITLE   swap.asm
	NAME    swap
	.8087

; MS-DOS System Function with Swaping - Front End
;
; MS-DOS System - Copyright (c) 1990,1,2 Data Logic Limited
;
; This code is subject to the following copyright restrictions:
;
; 1.  Redistribution and use in source and binary forms are permitted
;     provided that the above copyright notice is duplicated in the
;     source form.
;
;    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/swap.asm,v 2.0 1992/05/21 16:49:54 Ian_Stewartson Exp $
;
;    $Log: swap.asm,v $
;	Revision 2.0  1992/05/21  16:49:54  Ian_Stewartson
;	MS-Shell 2.0 Baseline release
;
;	Revision 2.0  1992/04/13  17:39:45  Ian_Stewartson
;	MS-Shell 2.0 Baseline release
;
;
; MODULE DEFINITION:
;
; This is the front end for the swapping version of system.  When linked
; in, it should be the first module in the load line so that it appears in
; memory immediately after the psp.  For example:
;
;   link swap+x1+x2+x3+system;
;
; This module has no user changeable features.  All selections are
; performed in the associated system function.
;
; If you want small model, you can only get small model data.  This
; requires this module to be compiled with the -DSMALL_DATA switch to the
; assembler.
;
; More details in system.c
;
; Author:
;	Ian Stewartson
;	Data Logic, Queens House, Greenhill Way
;	Harrow, Middlesex  HA1 1YR, UK.
;	istewart@datlog.co.uk or ukc!datlog!istewart
;
;

;
; Segment declarations
;

SWAP_TEXT	segment word public 'CODE'
SWAP_TEXT	ends

_DATA		segment word public 'DATA'
_DATA		ends

CONST		segment word public 'CONST'
CONST		ends

_BSS		segment word public 'BSS'
_BSS		ends

DGROUP		group	CONST, _BSS, _DATA

;
; Declare external functions and data
;
	extrn	__maperror:far
	extrn	_errno:word
	extrn	__psp:word

;
; Start of the spawn function
;

SWAP_TEXT	segment
		assume  cs: SWAP_TEXT, ds: NOTHING, ss: DGROUP

;
; For this function, all the code and data space are in the code space
;
		public	_cmd_line
		public	_path_line
		public	_SW_intr
		public	_SW_Blocks
		public	_SW_fp
		public	_SW_EMstart
		public	_SW_Mode
		public	_SW_EMSFrame
		public	_SW_Int00
		public	_SW_Int23
		public	_SW_XMS_Driver
		public	_SW_XMS_Gversion
		public	_SW_XMS_Allocate
		public	_SW_XMS_Free
		public	_SW_XMS_Available
		public	_SW_I23_InShell


_cmd_line	db	129 dup (?)	; Command line
_path_line	db	80 dup (?)	; Path line
_SW_Blocks	dw	0		; Number of blocks to read/write
_SW_fp		dw	0ffffH		; File ID
_SW_EMstart	dd	0100000H	; Default Extended Mem start
_SW_Mode	dw	0		; Type of swapping to do
					;   1 - disk
					;   2 - Extended memory
					;   3 - EMS driver
					;   4 - XMS driver
_SW_EMSFrame	dw	0		; EMS Frame segment
_SW_intr	dw	0		; Interrupt 23 detected.
_SW_XMS_Driver	dd	0		; XMS Driver Interface
_SW_I23_InShell	db	0		; In shell flag for Interrupt 23


;
; Some addition variables
;

SW_LMstart	dd	0		; Low Mem start for Extended Mem swap
N_mcb		dw	0		; Start write address
Result		dw	0		; Return value

;
; Stack save pointers
;

S_ss		dw	0			; Save Stack pointers
S_sp		dw	0
S_di		dw	0			; Save DI, SI
S_si		dw	0
S_ds		dw	0			; Save the original DS

;
; Two blank FCB
;

FCB1		dw	16	dup (?)
FCB2		dw	16	dup (?)

;
; XMS Driver Move structure
;

XMS_DIF		equ	$
XMS_Length	dd	0			; Number of bytes
XMS_SHandle	dw	0			; Source Handler
XMS_Soffset	dd	0			; Source Offset
XMS_DHandle	dw	0			; Destination Handler
XMS_Doffset	dd	0			; Destination Offset

;
; Extended Memory Global Descriptor tables
;
		org	XMS_DIF
GD_table	equ	$
GDT_Dummy	dw	4	dup (0)		; Dummy
GDT_self	dw	4	dup (0)		; For self
GDT_src		equ	$			; Source
		dw	04000H			; Length - 16K bytes
GDT_src_low	dw	0			;     Low Order address
GDT_src_high	db	0			;     High Order address
		db	093h			;     Access Rights
		dw	0			;     Reserved
GDT_dest	equ	$			; Destination
		dw	04000H			;     Length - 16K bytes
GDT_dest_low	dw	0			;     Low Order address
GDT_dest_high	db	0			;     High Order address
		db	093h			;     Access Rights
		dw	0			;     Reserved
GDT_bios	dw	4	dup (0)		; Bios
GDT_stack	dw	4	dup (0)		; Stack

;
; Execute interrupt structure
;

exec_parms	equ	$
exec_env	dw	0
		dw	offset _cmd_line	; Command line address
exec_cseg	dw	?
		dw	offset FCB1		; FCB1 address
exec_f1seg	dw	?
		dw	offset FCB2		; FCB1 address
exec_f2seg	dw	?

Swap_PANIC	db	'PANIC: Swap file re-load error - REBOOT', 0aH, 0dH
		db	'$'

Swap_DZERO	db	'PANIC: Divide by zero', 0aH, 0dH
		db	'$'

;
; OK - exec requires a local stack, cause some programs overwrite it
;
		even
		db	398 dup (0)
Local_Stack:
		dw	0

;
; Code starts
;
	public	_SA_spawn

SA_spawn1	proc	far
	mov	ds, word ptr cs:exec_env	; Load Env seg.
	xor	si, si				; Clear start offset

;
; Copy into Env Seg
;

$Copy_Env:
ifndef SMALL_DATA
	les	bx, dword ptr ss:[bp + 6]	; Check for end of loop
	mov	ax, word ptr es:[bx + 0]
	or	ax, word ptr es:[bx + 2]
else
	mov	dx, word ptr cs:S_ds		; Get original DS
	mov	es, dx				; and dump it in dx for later
	mov	bx, word ptr ss:[bp + 6]	; Check for end of loop
	mov	ax, word ptr es:[bx + 0]
	or	ax, ax
endif
	je	$Copy_End

;
; Save start address
;
; For small data model,the DX register contains the original DS
;

ifndef SMALL_DATA
	add	word ptr ss:[bp + 6], 4		; Increment environment by 4

	mov	cx, word ptr es:[bx + 0]	; Load address of cur Env string
	mov	ax, word ptr es:[bx + 2]	; into es:bx
else
	add	word ptr ss:[bp + 6], 2		; Increment environment by 2

	mov	cx, word ptr es:[bx + 0]	; Load address of cur Env string
	mov	ax, dx				; into es:bx
endif
	mov	es, ax
	mov	bx, cx

;
; Copy this value
;

$Copy_Val:
	mov	al, byte ptr es:[bx]	; Copy across
	mov	byte ptr ds:[si], al
	inc	bx			; Increment pointers
	inc	si
	or	al, al
	jne	$Copy_Val
	jmp	$Copy_Env

;
; Set up exec parameter block	 - DS is on stack
;
$Copy_End:
	xor	ax, ax
	mov	word ptr ds:[si], ax	; Terminate environment
	add	si, 2

;
; Set up new program length
;
	add	si, 16			; Round up paras
	mov	dx, si			; Save end offset in DX
	mov	bx, ds

	mov	cl, 4
	shr	si, cl			; # paras used by Env
	add	si, bx			; End para number

	mov	bx, word ptr cs:N_mcb	; Load our MCB address in BX
	mov	ax, bx
	inc	ax
	sub	si, ax
	mov	cx, si			; Save new max paras in CX

;
; Use interrupt 4a to shrink memory.  First release all memory above us.
;
	push	ax
	push	cx			; Save Max paras and location
	mov	ds, bx			; Set up the segement for MCB
	mov	cx, word ptr ds:3	; Get the MCB length

; Are we the only one in the chain?

	cmp	byte ptr ds:0, 'Z'	; End of chain ?
	jz	$Shrink_First

;
; Loop round releasing memory blocks
;
;	CX - original length of block;
;	DS - segement of the previous block
;
$Shrink_Next:
	mov	ax, ds			; Move to the next block
	add	cx, ax
	inc	cx
	mov	ds, cx

	cmp	byte ptr ds:0, 'Z'	; End of chain ?
	jz	$Shrink_First

	mov	cx, word ptr ds:3	; Save the length of this block

	mov	ax, ds			; Advance to the block itself
	inc	ax
	mov	es, ax			; Set up Block address

	mov	ah, 049H
	int	021H
	jmp	$Shrink_Next

;
;	Shrink the PSP segment
;

$Shrink_First:
	pop	cx
	pop	ax
        mov	es, ax			; Set PSP address
	mov	bx, cx			; Set max length
	mov	ah, 04aH
	int	021H

;
; Execute function
;

	mov	word ptr cs: S_sp, sp	; Save the current stack
	mov	word ptr cs: S_ss, ss

;
; Move to the local stack so that it doesn't get overwritten.
;
	mov	ax, cs
	cli
	mov	sp, offset Local_Stack
	mov	ss, ax
	sti

; Clear out Interrupts

	mov	ah, 00bH		; Check Keyboard status
	int	021H

;
;  Check for interrupt 23 detected
;
	mov	ax, word ptr cs:_SW_intr
	or	ax, ax
	jz	$I23_Cf			; No - continue;

;
; Interrupt 23 detected - abort
;
	mov	ax, cs			; Set up for reload
	cli
	mov	sp, offset Local_Stack
	mov	ss, ax
	sti

	mov	ds, word ptr cs:S_ds	; Restore DS
	xor	ax, ax
	jmp	$Exec_Complete

;
; No interrupts - continue
;
$I23_Cf:
	mov	ax, cs			; Set up segments
	mov	es, ax
	mov	ds, ax

	mov	ax, 04b00H		; Load and execute function
	mov	dx, offset _path_line	; Load path
	mov	bx, offset exec_parms	; Load the execute structure
	mov	byte ptr cs:_SW_I23_InShell, 1	; Set not shell flag
	int	021H
	mov	byte ptr cs:_SW_I23_InShell, 0	; Set in shell flag

; Disable interrupts while we restore the stack to the local one

	mov	ax, cs
	cli
	mov	sp, offset Local_Stack
	mov	ss, ax
	sti

;
; Did an error occur?
;
	jnc	$Exec_OK

;
; Error
;

$Map_error:
	mov	ds, word ptr cs:S_ds	; Restore DS
	mov	ah, al
	call	far ptr __maperror	; Map the error

$Exec_Error:
	mov	ax, 0FFFFH
	jmp	$Exec_Complete

;
; No - get the exit code and check for interrupts
;

$Exec_OK:
	mov	ax, 04d00H
	int	021H
	dec	ah			; Interrupt termination ?
	jnz	$Exec_OK1

	inc	word ptr ds:_SW_intr	; Set Interrupt 23 detected.

$Exec_OK1:
	xor	ah, ah

;
; Save the result code
;

$Exec_Complete:
	mov	word ptr cs:Result, ax		; Save response

;
; Very Dangerous - Restore Environment
;
;	 Seek to 0x4000 in file
;
	mov	bx, word ptr cs:_SW_fp		; Load File Handler
	mov	ax, word ptr cs: _SW_Mode	; Skip if not disk
	dec	ax
	jnz	$Seek_OK

; Seek in file to skip 16K

	mov	dx, 04000H
	call	$Seek_Disk

;
;	 Load from N_mcb:0x4000 to end of file.
;

$Seek_OK:
	mov	si, word ptr cs:_SW_Blocks	; Load number of transfers
	dec	si				; Skip first block

;
; set up ES register with start of load
;
	mov	ax, word ptr cs:N_mcb	; Load the start address
	add	ax, 0400H
	mov	ds, ax

; load up extended memory GDT for destination

	call	$GDT_reload
	call	$Inc_Extend			; Increment addresses by 16K

;
; Check for end of copy	- BX - File Handler for disk
;

$Read_loop:
	or	si, si
	je	$Read_Complete

; OK - Copy next 0x4000 bytes - switch on device

	mov	ax, word ptr cs: _SW_Mode
	dec	ax
	jz	$R_disk
	dec	ax
	jz	$R_extend
	dec	ax
	jz	$R_expand

;
; Read from XMS driver.  In this case, we do one read and let the driver
; sort out the blocking
;
	call	$Read_XMS
	jmp 	$Read_Complete

; Read from disk

$R_disk:
	call	$Read_disk
	jmp	$Read_loop

; Read from extended memory

$R_extend:
	call	$Read_extend
	jmp	$Read_loop

; Read from expanded memory

$R_expand:
	call	$Read_EMS
	jmp	$Read_loop

;
; Re-load is now complete, Restore original stack which has just been
; reloaded.  BX contains FP
;

$Read_Complete:
	cli
	mov	sp, word ptr cs: S_sp		; Save the current stack
	mov	ss, word ptr cs: S_ss
	sti

;  Save exit code

	push	word ptr cs:Result		; Save response
	push	word ptr cs:_SW_intr		; and interrupt flag

;
; Read in the first block - BX - File Handler
;

	mov	ax, word ptr cs: _SW_Mode	; Skip if not disk
	dec	ax
	jnz	$Seek1_OK

; Seek to 0 in file

	xor	dx, dx
	call	$Seek_Disk

;
;	 Load one block at N_mcb:0x0000
;
$Seek1_OK:
	mov	ds, word ptr cs:N_mcb		; Load the start address
	call	$GDT_reload			; Load the GDT for extend mem

	mov	ax, word ptr cs: _SW_Mode	; Skip if not disk
	dec	ax
	jz	$R1_Disk
	dec	ax
	jz	$R1_Extend
	dec	ax
	jz	$R1_Expand

	mov	si, 1				; Read one block
	call	$Read_XMS
	jmp	$Read1_OK

$R1_Disk:
	call	$Read_disk
	jmp	$Read1_OK

$R1_Extend:
	call	$Read_extend
	jmp	$Read1_OK

$R1_Expand:
	mov	si, word ptr cs:_SW_Blocks	; Read first block
	call	$Read_EMS

;
; Complete - load error code and return
;

$Read1_OK:
	pop	word ptr cs:_SW_intr		; Restore interrupt flag
	pop	ax

;
; Exit function - Restore Control Interrupt handler
;

$SA_spawn_Exit:

	mov	di, word ptr cs:S_di		; Restore saved registers
	mov	si, word ptr cs:S_si
	mov	ds, word ptr cs:S_ds

	mov	sp, bp
	pop	bp
	ret

SA_spawn1	endp

;
; READ XMS DRIVER FUNCTION
;
;	BX - file handler
;	SI - Block count
;	DS - Output data segement
;
$Read_XMS	proc	near
	xor	ax, ax
	mov	word ptr cs:XMS_SHandle, bx	; Source - XMS
	mov	word ptr cs:XMS_DHandle, ax	; Dest - normal memory

	mov	word ptr cs:XMS_Soffset, ax	; Source offset - zero
	mov	word ptr cs:XMS_Soffset + 2, ax

	mov	word ptr cs:XMS_Doffset, ax	; Dest offset DS:0
	mov	ax, ds
	mov	word ptr cs:XMS_Doffset + 2, ax

	cmp	si, 1				; If first block, the
	jz	$Read_X1			; source offset is
						; 4000H
	mov	word ptr cs:XMS_Soffset, 04000H

;
; Set up number of bytes: si * 16 * 1024
;

$Read_X1:
	mov	ax, si
	mov	dx, si
	mov	cl, 14
	shl	ax, cl
	mov	cl, 2
	shr	dx, cl
	mov	word ptr cs:XMS_Length, ax	; Load number of bytes
	mov	word ptr cs:XMS_Length + 2, dx

	mov	ah, 0BH				; Set up parameters
	mov	dx, cs
	mov	ds, dx
	mov	si, offset XMS_DIF
	call	cs:[_SW_XMS_Driver]
	or	ax, ax
	jnz 	$Read_XMS1
	jmp	Load_Error			; XMS error - abort

$Read_XMS1:
	ret

$Read_XMS	endp

;
; READ DISK FUNCTION
;
;	BX - file handler
;	SI - Block count
;	DS - Output data segement
;

$Read_disk	proc	near

	mov	ax, 03f00H		; Set up to read
	mov	cx, 04000H		; Load count
	xor	dx, dx			; Clear start address

	int	021H			; Read the data

	jnc	$Read_OK		; NO - abort
	jmp	Load_Error		; Abort - swap file error

;
; Read OK - next block
;

$Read_OK:
	dec	si			; Decrement block count
	mov	ax, ds			; Increment offset
	add	ax, 0400H
	mov	ds, ax
	ret

$Read_disk	endp

;
; READ EMS FUNCTION
;
;	BX - file handler
;	SI - Block count - counts from max
;	DS - Output data segement
;

$Read_EMS	proc	near

	call	$map_ems_page		; Map in the current EMS page
	jnz	Load_Error

	push	ds			; Save DS and SI
	push	si
	mov	ax, ds
	mov	es, ax
	mov	ds, word ptr cs:_SW_EMSFrame	; Set Dest Seg
	xor	si, si			; Clear start
	xor	di, di
	mov	cx, 02000H		; move 16K
	pushf				; Save direction flag
	cld
    rep movsw
	popf				; Restore direction flag
	pop	si			; And DS, SI
	pop	ds
	jmp	$Read_OK		; Increment DS and dec SI

$Read_EMS	endp

;
; MAP IN THE CURRENT EMS PAGE
;
;	BX - file handler
;	SI - Block count - counts from max
;	DS - Output data segement
;

$map_ems_page	proc	near

	push	bx			; Need to save BX
	mov	ax, 04400h		; Map into physical page zero
	mov	dx, bx			; Set up handler
	mov	bx, word ptr cs: _SW_Blocks
	sub	bx, si

	int	067H
	pop	bx

	or	ah, ah
	ret

$map_ems_page	endp

;
; DISK SEEK FUNCTION
;
;	BX - file handler
;	DX - offset
;
$Seek_Disk	proc	near

	mov	ax, 04200H		; Set seek
	xor	cx, cx
	int	021H
	jc	Load_Error		; Abort - swap file error
	ret

$Seek_Disk	endp

;
; PANIC - Abort
;

Load_Error	proc	near

	mov	di, offset Swap_PANIC
	mov	bx, cs
	mov	ds, bx

;
;  Display message function
;
;  DS:DI message
;  AX is available
;

	mov	ah, 08H			; Get foreground colour
	xor	bx, bx
	int	10H
	mov	bl, ah
	and	bl, 07h

;
; Loop until a $ is hit, outputting the characters
;
I24D:
	mov	al, byte ptr ds:[di]
	cmp	al, '$'
	jz	$Wait_L

	push	di
	mov	ah, 0EH
	int	10H
	pop	di
	inc	di
	jmp	I24D

$Wait_L:
	sti
	hlt
	jmp	$Wait_L

Load_Error	endp

;
;  WRITE EXTENDED MEMORY
;
;	SI - Block count
;
$Write_extend	proc	near

	push	si			; Save SI (block counter)
	mov	cx, 02000H		; Copy a 16K block
	mov	ax, cs			; Set up GDT address
	mov	es, ax
	mov	si, offset GD_table

	mov	ah, 087H		; EMS function
	int	015H
	pop	si
	ret

$Write_extend	endp

;
;  READ FROM EXTENDED MEMORY
;
;	SI - Block count
;

$Read_extend	proc	near

	call	$Write_extend
	jc	Load_Error		; NO - abort

	dec	si			; Decrement block count

$Read_extend	endp

;
; INCREMENT Extended MEMORY GDT
;
;	AX - used
;
$Inc_Extend	proc	near

	mov	ax, 04000H		; Increment address by 16K
	add	word ptr cs:GDT_dest_low, ax
	adc	byte ptr cs:GDT_dest_high, 0
	add	word ptr cs:GDT_src_low, ax
	adc	byte ptr cs:GDT_src_high, 0
	ret

$Inc_Extend	endp

;
; LOAD SOURCE GDT ADDRESS
;
;	AX - low order
;	DL - high order
;
$GDT_src_load	proc	near


	mov	word ptr cs:GDT_src_low, ax
	mov	byte ptr cs:GDT_src_high, dl
	ret

$GDT_src_load	endp

;
; LOAD DESTINATION GDT ADDRESS
;
;	AX - low order
;	DL - high order
;
$GDT_dest_load	proc	near

	mov	word ptr cs:GDT_dest_low, ax
	mov	byte ptr cs:GDT_dest_high, dl
	ret

$GDT_dest_load	endp

;
; LOAD the GDT for reloading
;

$GDT_reload	proc	near
	mov	ax, word ptr cs:_SW_EMstart 	; Load Full start address
	mov	dl, byte ptr cs:_SW_EMstart + 2
	call	$GDT_src_load

	mov	ax, word ptr cs:SW_LMstart 	; Load Full start address
	mov	dl, byte ptr cs:SW_LMstart + 2
	call	$GDT_dest_load
	ret
$GDT_reload	endp

;
; CONTROL C INTERRUPT HANDLER - IGNORE
;

_SW_Int23	proc	far
	inc	word ptr cs:_SW_intr	; Set Interrupt 23 detected.
	cmp	byte ptr cs:_SW_I23_InShell, 0	; are we in the shell?
	jz	$SA_Ins

; In another program - move the stack around

	stc
	ret


; In shell - ignore interrupt 23 for the moment

$SA_Ins:
	iret

_SW_Int23	endp

;
; DIVIDE BY ZERO INTERRUPT HANDLER - Output message
;

_SW_Int00	proc	far

	mov	ax, 00900H
	mov	dx, offset Swap_DZERO
	mov	bx, cs
	mov	ds, bx
	int	021H

	mov	ax, 04CFFh		; Exit
	int	021H

_SW_Int00	endp

;
;  Start of overwrite area for environment.  Align on a paragraph
;
;  Also the XMS driver functions used by SH3.C live here
;
		ALIGN	16
Env_OWrite:

;
; XMS INTERFACE
;
; Get Version number.  Return the release number in AX
;

_SW_XMS_Gversion	proc	far

	push	bp			; Save stack info
	mov	bp, sp

	xor	ax, ax
	call	cs:[_SW_XMS_Driver]

	mov	sp, bp
	pop	bp
	ret

_SW_XMS_Gversion	endp

;
; Allocate N kbytes.  Return the Handler in AX or -1 and error code in
; errno.
;
; Size will be in bp + 6.
;

_SW_XMS_Allocate	proc	far

	push	bp			; Save stack info
	mov	bp, sp

	mov	dx, word ptr ss:[bp + 6]
	mov	ah, 09H
	call	cs:[_SW_XMS_Driver]
	or	ax, ax
	jnz	$SW_A1

;
; Allocate Failed - return error code
;
	xor	ax, ax
	dec	ax
	xor	bh, bh
	mov	word ptr ds:_errno, bx	; Save error code
	jmp	$SW_A2

;
; Allocate OK - return handler
;

$SW_A1:
	mov	ax, dx

$SW_A2:
	mov	sp, bp
	pop	bp
	ret

_SW_XMS_Allocate	endp

;
; Release handler.  Return 0 or error code.
;
; Handler will be in bp + 6.
;

_SW_XMS_Free 		proc	far

	push	bp			; Save stack info
	mov	bp, sp

	mov	dx, word ptr ss:[bp + 6]
	mov	ah, 0AH
	call	cs:[_SW_XMS_Driver]
	or	ax, ax
	jnz	$SW_F1
;
; Free Failed - return error code
;
	mov	al, bl
	jmp	$SW_F2

;
; Free OK - return zero
;
$SW_F1:
	xor	ax, ax

$SW_F2:
	mov	sp, bp
	pop	bp
	ret

_SW_XMS_Free 		endp

;
; Get available memory space.  Return 0 if failed.
;


_SW_XMS_Available	proc	far

	push	bp			; Save stack info
	mov	bp, sp
	mov	ah, 08H
	call	cs:[_SW_XMS_Driver]
	or	bl, bl
	jz	$SW_Avail1

;
; Free Failed - return zero
;
	xor	ax, ax

$SW_Avail1:
	mov	sp, bp
	pop	bp
	ret

_SW_XMS_Available	endp

;
; Main entry point
;

_SA_spawn	proc	far

	push	bp
	mov	bp, sp

;
; Entry Offsets
;
;	Environment      = 6
;

	mov	word ptr cs:S_di, di		; Save registers
	mov	word ptr cs:S_si, si
	mov	word ptr cs:S_ds, ds

;
; Save the length of the current MCB block;
;

	mov	ax, word ptr ds:__psp
	dec	ax
	mov	word ptr cs:N_mcb, ax		; Save MCB address for swap out

; Calculate low mem start for extended memory

	mov	bx, ax				; Save copy
	mov	cl, 4				; mult low order by 16
	shl	ax, cl
	mov	word ptr cs:SW_LMstart, ax	; Save low order
	mov	cl, 12				; div by 16 ** 3
	shr	bx, cl
	mov	byte ptr cs:SW_LMstart + 2, bl	; Save low order

;
; Set up Environment segment in execute structure
;

	mov	bx, cs
	mov	ax, offset Env_OWrite
	mov	cl, 4
	shr	ax, cl
	add	ax, bx
	mov	word ptr cs:exec_env, ax	; Save Env seg.

;
; Set up rest of execute structure
;

	mov	word ptr cs:exec_cseg, cs	; Command line address
	mov	word ptr cs:exec_f1seg, cs	; FCB 1 address
	mov	word ptr cs:exec_f2seg, cs	; FCB 2 address

;
; Generate the FCBs
;

	mov	ax, cs		; Set up segments
	mov	ds, ax
	mov	es, ax

	mov	ax, 02901H	; Set up FCB interrupt
	mov	si, offset _cmd_line + 1
	mov	di, offset FCB1	; FCB 1;

	int	021H		; Execute the interrupt

	mov	ax, cs		; Set up segment
	mov	es, ax

	mov	ax, 02901H	; Reset AX cause errors are ignored
	mov	di, offset FCB2	; FCB 2;

	int	021H		; Execute the interrupt

;
; Copy out to the swap file
;

	mov	bx, word ptr cs:_SW_fp		; Load file handler
	mov	si, word ptr cs:_SW_Blocks	; Load Number of blocks to read
	mov	ax, word ptr cs:N_mcb		; Load the start address
	push	ax

; load up extended memory GDT for destination

	mov	ax, word ptr cs:_SW_EMstart
	mov	dl, byte ptr cs:_SW_EMstart + 2
	call	$GDT_dest_load

;
; set up DS register with start of start copy
;

	pop	ax
	mov	ds, ax
	mov	ax, word ptr cs:SW_LMstart 	; Load Full start address
	mov	dl, byte ptr cs:SW_LMstart + 2
	call	$GDT_src_load

;
; Check for end of copy - BX contains the file handler for disk write
;

$Write_loop:
	or	si, si
	jnz	$Write_L1
	jmp	$Write_Complete

; OK - Copy next 0x4000 bytes - switch on device

$Write_L1:
	mov	ax, word ptr cs: _SW_Mode
	dec	ax
	jz	$W_disk
	dec	ax
	jnz	$Write_L2
	jmp	$W_extend
$Write_L2:
	dec	ax
	jnz	$W_xms
	jmp	$W_expand

;
; Write to XMS driver.  In this case, we do one write and let the driver
; sort out the blocking
;
$W_xms:
	xor	ax, ax
	mov	word ptr cs:XMS_SHandle, ax	; Source - normal memory
	mov	word ptr cs:XMS_DHandle, bx	; Dest - XMS

	mov	word ptr cs:XMS_Doffset, ax	; Dest offset - zero
	mov	word ptr cs:XMS_Doffset + 2, ax

	mov	word ptr cs:XMS_Soffset, ax	; Source offset DS:0
	mov	ax, ds
	mov	word ptr cs:XMS_Soffset + 2, ax

;
; Set up number of bytes SW_Block * 16 * 1024
;

	mov	ax, si
	mov	dx, si
	mov	cl, 14
	shl	ax, cl
	mov	cl, 2
	shr	dx, cl
	mov	word ptr cs:XMS_Length, ax	; Load number of bytes
	mov	word ptr cs:XMS_Length + 2, dx

	mov	ah, 0BH				; Set up parameters
	mov	dx, cs
	mov	ds, dx
	mov	si, offset XMS_DIF
	call	cs:[_SW_XMS_Driver]
	or	ax, ax
	jnz 	$Write_Complete

; XMS error - abort

	mov	ah, bl
	jmp	$Write_error

;
; Write to disk
;
$W_disk:
	call	$Write_disk

; Increment counter

$Write_Incr:
	dec	si			; Decrement block count
	mov	ax, ds			; Increment offset
	add	ax, 0400H
	mov	ds, ax
	jmp	$Write_loop

; Write to extended memory

$W_extend:
	call	$Write_extend
	jc	$Write_error		; NO - abort

	dec	si			; Decrement block count
	call	$Inc_Extend
	jmp	$Write_loop

; Write to expanded memory
;	BX - handler
;	SI - count
;	DS - source segment
;
$W_expand:
	call	$map_ems_page		; Map in the current EMS page
	jnz	$Write_error

	push	ds			; Save DS and SI
	push	si
	mov	es, word ptr cs:_SW_EMSFrame	; Set Dest Seg
	xor	si, si			; Clear start
	xor	di, di
	mov	cx, 02000H		; move 16K
	pushf				; Save direction flag
	cld
    rep movsw
	popf				; Restore direction flag
	pop	si			; And DS, SI
	pop	ds
	jmp	$Write_Incr		; Increment DS and dec SI

;
; Error - abort.  The error code is in AH.
;

$Write_error:
	mov	ds, word ptr cs:S_ds	; Restore DS
	mov	al, ah
	xor	ah, ah
	mov	word ptr ds:_errno, ax	; Save error code

$Write_Error1:
	mov	ax, 0FFFEH
	jmp	$SA_spawn_Exit		; Exit

;
; Swap file is now written, set up environment.  If this was a partial
; write, we need to write the first 4K.
;

$Write_Complete:
		jmp	SA_spawn1
_SA_spawn	endp

;
; WRITE DISK FUNCTION
;
;	BX - file handler
;	SI - Block count
;	DS - Output data segement
;

$Write_disk	proc	near

	mov	ax, 04000H		; Set up to write
	mov	cx, ax			; Load count
	xor	dx, dx			; Clear start address
	push	bx			; Save FP
	push	si			; Save count and Data Segment

	int	021H			; Write the data

	pop	si			; Restore Regs
	pop	bx
	jnc	$Write_disk1		; NO error - continue

;
; Error - abort
;
	mov	ds, word ptr cs:S_ds	; Restore DS
	mov	ah, al
	call	far ptr __maperror	; Map the error
	jmp	$Write_Error1

; Check for 16K write

$Write_disk1:
	cmp	ax, 04000H
	jnz	$Write_disk2
	ret

$Write_disk2:
	mov	ax,01c1cH		; Set disk full
	jmp	$Write_error		; NO - abort

$Write_disk	endp

;
; END OF SWAPPER
;
SWAP_TEXT	ends
		end

	TITLE   sh0.asm
	NAME    sh0
	.8087

; MS-DOS SHELL - Swapper
;
; MS-DOS SHELL - Copyright (c) 1990,4 Data Logic Limited
;
; This code is subject to the following copyright restrictions:
;
; 1.  Redistribution and use in source and binary forms are permitted
;     provided that the above copyright notice is duplicated in the
;     source form and the copyright notice in file sh6.c is displayed
;     on entry to the program.
;
; 2.  The sources (or parts thereof) or objects generated from the sources
;     (or parts of sources) cannot be sold under any circumstances.
;
;    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/sh0.asm,v 2.6 1994/08/25 20:58:47 istewart Exp $
;
;    $Log: sh0.asm,v $
;	Revision 2.6  1994/08/25  20:58:47  istewart
;	MS Shell 2.3 Release
;
;	Revision 2.5  1994/01/11  17:55:25  istewart
;	Release 2.3 Beta 0 patches
;
;	Revision 2.4  1993/11/09  10:39:49  istewart
;	Beta 226 checking
;
;	Revision 2.3  1993/08/25  16:03:57  istewart
;	Beta 225 - see Notes file
;
;	Revision 2.2  1993/06/14  11:00:54  istewart
;	More changes for 223 beta
;
;	Revision 2.1  1992/07/16  14:33:34  istewart
;	Beta 212 Baseline
;
;	Revision 2.0  1992/04/13  17:39:45  Ian_Stewartson
;	MS-Shell 2.0 Baseline release
;
;
;

;
; Segment declarations
;

SH0_TEXT	segment word public 'CODE'
SH0_TEXT	ends

_DATA		segment word public 'DATA'
_DATA		ends

CONST		segment word public 'CONST'
CONST		ends

_BSS		segment word public 'BSS'
_BSS		ends

DGROUP		group	CONST, _BSS, _DATA
;DGROUP		group	_BSS, _DATA

C_ETEXT		segment word public 'ENDCODE' 
C_ETEXT		ends 

;
; Declare external functions and data
;

IFDEF __TURBOC__
	extrn	__IOERROR:far
ELSE
	extrn	__maperror:far
ENDIF
	extrn	_errno:word
	extrn	__psp:word

;
; Declare end of text variable.  The ENDCODE segment appears to be the last
; code segment loaded by the Microsoft loader
;

C_ETEXT		segment word public 'ENDCODE' 
		public	cetext
cetext		equ	$
C_ETEXT		ends 

;
; Start of the spawn function
;

SH0_TEXT	segment
		assume  cs: SH0_TEXT, ds: NOTHING, ss: DGROUP

;
; For this function, all the code and data space are in the code space
;
		public	_cmd_line
		public	_path_line
		public	_SW_intr
		public	_SW_Blocks
		public	_SW_SBlocks
		public	_SW_fp
		public	_SW_EMstart
		public	_SW_Mode
		public	_SW_EMSFrame
		public	_SW_Int00
		public	_SW_Int23
		public	_SW_Int24
		public	_SW_XMS_Driver
		public	_SW_XMS_Gversion
		public	_SW_XMS_Allocate
		public	_SW_XMS_Free
		public	_SW_XMS_Available 
		public	_Poll_Keyboard
		public	_etext
		public	_SW_Pwrite
		public	_SW_I23_InShell
		public	_SW_MinESpace


_cmd_line	db	129 dup (?)	; Command line
_path_line	db	80 dup (?)	; Path line
_etext		dw	seg cetext	; End of text segment
_SW_Blocks	dw	0		; Number of blocks to read/write
_SW_SBlocks	dw	0		; Short Number of blocks to read/write
_SW_fp		dw	0ffffH		; File ID
_SW_Pwrite	dw	0		; Partial write to disk?
_SW_EMstart	dd	0100000H	; Default Extended Mem start
_SW_Mode	dw	0		; Type of swapping to do
					;   1 - disk
					;   2 - Extended memory
					;   3 - EMS driver
					;   4 - XMS driver
_SW_EMSFrame	dw	0		; EMS Frame segment
_SW_intr	dw	0		; Interrupt 23 detected.
_SW_MinESpace	dw	0		; Minimum Environment space
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
	les	bx, dword ptr ss:[bp + 6]	; Check for end of loop
	mov	ax, word ptr es:[bx + 0]
	or	ax, word ptr es:[bx + 2]
	je	$Copy_End

;
; Save start address
;
	add	word ptr ss:[bp + 6], 4		; Increment environment by 4

	mov	cx, word ptr es:[bx + 0]	; Load address of cur Env string
	mov	ax, word ptr es:[bx + 2]	; into es:bx
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

	cmp	si, word ptr cs:_SW_MinESpace	;	Enough space ?
	jge	$NoMoreE
	mov	si, word ptr cs:_SW_MinESpace

;
; Set up new program length
;
$NoMoreE:
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
IFDEF __TURBOC__
	xor	ah, ah
	push	ax
	call	far ptr __IOERROR	; Map the error
ELSE
	mov	ah, al
	call	far ptr __maperror	; Map the error
ENDIF

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
	call	I24_Display
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
; INTERRUPT 24 - ERROR HANDLER - Output message
;
;    AH - Bit  7	= 0 Disk error
;			= 1 FAT error if block device
;			    Error code in DI if character device
;	  Bit  6	UNUSED
;	  Bit  5	= 1 Ignore allowed
;	  Bit  4	= 1 Retry allowed
;	  Bit  3	= 1 Fail allowed
;	  Bits 2 & 1	= Disk Area
;	  Bit  0	= 1 Writing error
;    AL 		= Disk drive number
;    DI			= Error code
;    BP:SI		= Header of device driver for which error occured
;
;
; Return
;    AL			= 0 Ignore Error
;			= 1 Retry operation
;			= 2 Abort program
;			= 3 Fail system call
;

I24_Errors	equ	$
I24_EC00:	db	'Write-protect error$'
I24_EC01:	db	'Unknown unit$'
I24_EC02:	db	'Drive not ready$'
I24_EC03:	db	'Unknown command$'
I24_EC04:	db	'CRC error$'
I24_EC05:	db	'Bad request structure length$'
I24_EC06:	db	'Seek error$'
I24_EC07:	db	'Unknown media type$'
I24_EC08:	db	'Sector not found$'
I24_EC09:	db	'Out of paper$'
I24_EC0A:	db	'Write fault$'
I24_EC0B:	db	'Read fault$'
I24_EC0C:	db	'General failure$'
I24_EC0D:	db	'Sharing violation$'
I24_EC0E:	db	'Lock violation$'
I24_EC0F:	db	'Invalid disk change$'
I24_EC10:	db	'FCB unavailable$'
I24_EC11:	db	'Sharing buffer overflow$'
I24_ECUK:	db	'Unknown error$'

;
; Error message address table
;

I24_ECTABLE:	dw	offset I24_EC00
		dw	offset I24_EC01
		dw	offset I24_EC02
		dw	offset I24_EC03
		dw	offset I24_EC04
		dw	offset I24_EC05
		dw	offset I24_EC06
		dw	offset I24_EC07
		dw	offset I24_EC08
		dw	offset I24_EC09
		dw	offset I24_EC0A
		dw	offset I24_EC0B
		dw	offset I24_EC0C
		dw	offset I24_EC0D
		dw	offset I24_EC0E
		dw	offset I24_EC0F
		dw	offset I24_EC10
		dw	offset I24_EC11
		dw	offset I24_ECUK

I24_ECON:	db	' when $'
I24_ECREAD:	db	'reading $'
I24_ECWRITE:	db	'writing $'
I24_ECDEVICE:	db	'device $'
I24_ECDISK:	db	'disk $'
I24_EABORT:	db	'Abort$'
I24_EFAIL:	db	', Fail$'
I24_EIGNORE:	db	', Ignore$'
I24_ERETRY:	db	', Retry$'
I24_EDRIVE:	db	'?:$'
I24_EQUESTION	db	'? $'
I24_RESPONSE:	db	' '
I24_ENL:	db	0dH, 0aH, '$'
I24_EBELL:	db	07H, '$'
I24_EDNAME:	db	'12345678:$'
I24_EXTECODE:	db	0dH, 0aH, '(Extended Code: '
I24_E_AL:	db	'  h'
		db	' Class: '
I24_E_BH:	db	'  h'
		db	' Action: '
I24_E_BL:	db	'  h'
		db	' Locus: '
I24_E_CH:	db	'  h)', 0dH, 0aH, '$'

;
; Save DS, ES, BX, CX, DX
;
_SW_Int24	proc	far
	push	ds		; Save registers
	push	es
	push	bx
	push	cx
	push	dx

	push	cs		; Set up data segment
	pop	ds

	mov	cx, ax		; Save the error information in CX

	mov	di, offset I24_ENL
	call	I24_Display
;
; Get extended error codes
;
	push	cx
	push	ds
	push	si
	push	bp
	mov	ah, 059H
	xor	bx, bx
	int	021H
	pop	bp
	pop	si
	pop 	ds
	pop	cx
	sub	ax, 13H
	mov	di, ax

;
;  Check inside message range
;
	cmp	di, 012H
	jb	SWI24a
	mov	di, 12H		;new 'unknown error' entry

;
; Write the error message
;

SWI24a:
	add	di, di

	mov	di, word ptr ds:I24_ECTABLE[di]
	call	I24_Display

;
; Output on message
;

	mov	di, offset I24_ECON
	call	I24_Display

;
; Output reading or write message
;

	mov	di, offset I24_ECWRITE
	test	ch, 01H
	jnz	SWI24b
	mov	di, offset I24_ECREAD

SWI24b:
	call	I24_Display

;
; Output device message
;

	test	ch, 080H
	jz	SWI24c

	mov	di, offset I24_ECDEVICE
	call	I24_Display

;
; Output device name - up to eight characters
;

	add	si, 0aH			; Move to device name
	push	ds
	mov	ds, bp
	xor	di, di			; Set counter

SWI24b1:
	mov	dl, byte ptr ds:[si]	; Get next character in name
	cmp	dl, ' '
	jz	SWI24d

	mov	byte ptr cs:[I24_EDNAME + di], dl

	inc	si
	inc	di
	cmp	di, 8
	jnz	SWI24b1
;
; Append a : and $
; 
SWI24d:
	pop	ds
	mov	byte ptr cs:[I24_EDNAME + di], ':'
	mov	byte ptr cs:[I24_EDNAME + 1 + di], '$'
	mov	di, offset I24_EDNAME
	jmp	SWI24e

;
; Write disk error
;
SWI24c:
	mov	di, offset I24_ECDISK
	call	I24_Display

	mov	dl, cl
	add	dl, 'A'
	mov	byte ptr cs:I24_EDRIVE, dl

	mov	di, offset I24_EDRIVE
SWI24e:
	call	I24_Display

;
; Get extended error codes
;
	push	cx
	push	ds
	mov	ah, 059H
	xor	bx, bx
	int	021H

;
; Save responses
;

	mov	byte ptr cs:I24_E_AL, al
	mov	byte ptr cs:I24_E_BL, bl
	mov	byte ptr cs:I24_E_BH, bh
	mov	byte ptr cs:I24_E_CH, ch
	pop	ds
	pop	cx

; Convert to display Hex.

	mov	di, offset I24_E_AL
	call	I24_Convert
	mov	di, offset I24_E_BL
	call	I24_Convert
	mov	di, offset I24_E_BH
	call	I24_Convert
	mov	di, offset I24_E_CH
	call	I24_Convert
	mov	di, offset I24_EXTECODE
	call	I24_Display
;
; Output Options 
;
	mov	di, offset I24_EABORT
	call	I24_Display

	test	ch, 020H		; Ignore allowed ?
	jz	SWI24f
	mov	di, offset I24_EIGNORE
	call	I24_Display

SWI24f:
	test	ch, 010H		; Retry allowed ?
	jz	SWI24g
	mov	di, offset I24_ERETRY
	call	I24_Display

SWI24g:
	test	ch, 08H			; Fail allowed ?
	jz	SWI24h
	mov	di, offset I24_EFAIL
	call	I24_Display

;
; Append a question mark.
;

SWI24h:
	mov	di, offset I24_EQUESTION
	call	I24_Display

;
; Get the valid key codes
;
SWI24j:
	xor	ax, ax			; Read a keyboard character
	int	16H
	and	al, 05fH		; Upper case

	xor	ah, ah			; Clear counter
	cmp	al, 'I'			; Ignore ?
	jnz	SWI24k
	test	ch, 020H
	jnz	SWI24n
SWI24k:
	inc	ah
	cmp	al, 'R'			; Retry ?
	jnz	SWI24l
	test	ch, 010H
	jnz	SWI24n
SWI24l:
	inc	ah
	cmp	al, 'A'			; Abort ?
	jz	SWI24n

	inc	ah
	cmp	al, 'F'			; Fail ?
	jnz	SWI24m
	test	ch, 08H
	jnz	SWI24n
SWI24m:
	mov	di, offset I24_EBELL
	call	I24_Display
	jmp	SWI24j

;
; OK - got code
;
SWI24n:
	mov	cl, ah
	mov	byte ptr ds:I24_RESPONSE, al
	mov	di, offset I24_RESPONSE
	call	I24_Display
	mov	ax, cx

;
; Are we in the shell ?
;
	cmp	byte ptr cs:_SW_I23_InShell, 0	; Are we in the shell ? 
	jnz	$SW_int24a		; No - no processing

	cmp	al, 02H			; Abort?
	jnz	$SW_int24a		; No - exit

	test	ah, 008h		; If fail allowed - convert to fail
	jz	$SW_int24a

	mov	al, 003h		; Fail system call

$SW_int24a:
	pop	dx			; Restore registers
	pop	cx
	pop	bx
	pop	es
	pop	ds		
	iret
_SW_Int24	endp

;
; Convert Hex code to display code
;
; ds:di Offset of code to replace
; ax is available
;
I24_Convert	proc	near
	push	cx
	mov	al, byte ptr ds:[di]	; Get the code
	mov	ah, al
	mov	cl, 4
	shr	ah, cl
	and	ah, 0fh

	cmp	ah, 10
	jb	I24_C1
	add	ah, 'A' - 10
	jmp	I24_C2
I24_C1:
	add	ah, '0'
I24_C2:
	mov	byte ptr ds:[di], ah

; Now LSB

	and	al, 0fh
	cmp	al, 10
	jb	I24_C3
	add	al, 'A' - 10
	jmp	I24_C4
I24_C3:
	add	al, '0'
I24_C4:
	mov	byte ptr ds:[di + 1], al
	pop	cx
	ret
I24_Convert	endp
;
;  Display message function for Interrupt 24 processing
;
;  DS:DI message
;  AX is available
;

I24_Display	proc	near
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
	jnz	I24Da
	ret

I24Da:
	push	di
	mov	ah, 0EH
	int	10H
	pop	di
	inc	di
	jmp	I24D

I24_Display	endp

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
; Read Keyboard via interrupt 21 function 6 with polling for DESQView support.
; Return Keyboard code in AL.
;

_Poll_Keyboard	proc	far
	push	bp
	mov	bp, sp
	push	ds			; Save the C registers
	push	es
	push	si
	push	di

ReadK1:
	mov	ah, 6			; Read the keyboard
	mov	dl, 0ffH
	int	021H
	jnz	ReadK2			; Got a keystroke!!

	mov	AX, 01000H		; Failed - Release timeslice.
	int	015H
	jmp	ReadK1

; Got a character - Get the keybaord status information

ReadK2:
	pop	di			; Restore stack
	pop	si
	pop	es
	pop	ds
	mov	sp, bp
	pop	bp
	ret	

_Poll_Keyboard	endp

;
; Main Entry point.  This part can be overwritten by the environment
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

;
;  Check for Partial write on disk swap file
;

	cmp	word ptr cs: _SW_Mode, 1	; Partial disk write ?
	jnz	SPart_Write
	cmp	word ptr cs: _SW_Pwrite, 0
	jz	SPart_Write

	mov	si, word ptr cs:_SW_SBlocks	; Load Number of blocks to read
	mov	ax, word ptr cs:_etext		; Load the start address

SPart_Write:
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
	cmp	word ptr cs: _SW_Mode, 1	; Partial disk write ?
	jnz	S1Part_Write
	cmp	word ptr cs: _SW_Pwrite, 0
	jz	S1Part_Write

	mov	ax, 04200H			; Set seek
	xor	dx, dx
	xor	cx, cx
	int	021H
	jnc	$Write_C1			; Abort - swap file error
	jmp	$Map_error

$Write_C1:
	mov	ax, word ptr cs:N_mcb		; Load the start address
	mov	ds, ax
	call	$Write_disk

;
; Go to the no-overwrite part.
;

S1Part_Write:
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
IFDEF __TURBOC__
	xor	ah, ah
	push	ax
	call	far ptr __IOERROR	; Map the error
ELSE
	mov	ah, al
	call	far ptr __maperror	; Map the error
ENDIF
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

SH0_TEXT	ends
		end
